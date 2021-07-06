import multiprocessing as mp
from sl.process_messages import Process_Messages
import sys


class Parser(mp.Process):
  def __init__(self, client_packages: mp.Queue):
    super().__init__()
    self.client_packages = client_packages

  def run(self):
    self.process_messages = Process_Messages()
    self.read_from_queue()

  def read_from_queue(self):
    # Bytes that have been recived, i.e. read from the client_packages queue
    recv_buffer = b''
    # Either 'header' or 'body', specifies what the bytes in 'recv_buffer'
    # are part of the message header or message body. 
    waiting_for = 'header'
    header_len = 5
    # wait_for_num_bytes specifies how many more bytes this server must
    # recive until the current header or body (depends on waiting_for) 
    # has completetly arrived and can be processed by the Process Messages
    # compoennt.
    wait_for_num_bytes = header_len
    # Type of message that is being recieved currently
    msg_ID = -1
    while True:
      # Wait until a new data has arrived. Then, append the recived data to
      # the recv_buffer.
      msg = self.client_packages.get()
      recv_buffer += msg
      while len(recv_buffer) >= wait_for_num_bytes:
        if waiting_for == 'header':
          # If all the bytes of a header have arrived, we must now wait for
          # the message body of this header. The length of this message body
          # is in the header.
          msg_ID, wait_for_num_bytes = self.deserialize_header(recv_buffer[:5])
          recv_buffer = recv_buffer[5:]
          waiting_for = 'body'
        elif waiting_for == 'body':
          # If all the bytes of the message body have arrived, handle_body
          # forwards the message body to one of the functions of the 
          # Process Messages component. Which function depends on the message ID
          self.handle_body(msg_ID, recv_buffer[:wait_for_num_bytes])
          recv_buffer = recv_buffer[wait_for_num_bytes:]
          waiting_for = 'header'
          wait_for_num_bytes = 5

  def deserialize_header(self, header_raw):
    return header_raw[0], int.from_bytes(header_raw[1:5], byteorder=sys.byteorder)

  def handle_body(self, msg_ID, body_raw):
    if msg_ID == 0:
      # Message with ID 0 is Initial SUT Information
      # Path to target executable (i.e. the executable that is emulated)
      # This message is sent first on every run
      body = body_raw.decode('utf-8')
      self.process_messages.initial_message(body)
    elif msg_ID == 1:
      # Message with ID 1 is New Coverage
      # body consists of: from addr (32 bit), to addr (32 bit),
      # input_size (32 bit), input (input_size bytes)
      from_addr = int.from_bytes(body_raw[:4], byteorder='little')
      to_addr = int.from_bytes(body_raw[4:8], byteorder='little')
      input_len = int.from_bytes(body_raw[8:12], byteorder='little')
      assert len(body_raw) - 12 == input_len
      inp = body_raw[12:]
      self.process_messages.save_previous_interesting_input(inp)
      self.process_messages.update_coverage(to_addr)
    elif msg_ID == 2:
      # Message with ID 2 is New Crash
      i = 0
      crash_ID = int(body_raw[i])
      i += 1
      crash_addr = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      origin_addr = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      input_length = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      crash_input = body_raw[i:i + input_length]
      i += input_length
      stack_frame_size = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      assert len(body_raw) - i == stack_frame_size * 4
      # split every 4 bytes
      stack_frames_pc = [int.from_bytes(
          body_raw[j:j + 4], byteorder='little') for j in range(i, len(body_raw), 4)]
      assert len(stack_frames_pc) == stack_frame_size
      self.process_messages.crashing_input(
          crash_ID, crash_addr, origin_addr, crash_input, stack_frames_pc)
    elif msg_ID == 3:
      # Message with ID 3 is Updated Fuzzer Statistics
      i = 0
      inputs_executed = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      total_crashes = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      max_depth = int.from_bytes(body_raw[i:i + 4], byteorder='little')
      i += 4
      assert i == len(body_raw)
      self.process_messages.update_fuzzer_stats(
          inputs_executed, total_crashes, max_depth)
    else:
      assert False, f"unknown {msg_ID=}"
