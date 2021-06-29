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
    recv_buffer = b''
    waiting_for = 'header'
    header_len = 5
    wait_for_num_bytes = header_len
    msg_ID = -1
    while True:
      msg = self.client_packages.get()
      recv_buffer += msg
      while len(recv_buffer) >= wait_for_num_bytes:
        if waiting_for == 'header':
          msg_ID, wait_for_num_bytes = self.deserialize_header(recv_buffer[:5])
          recv_buffer = recv_buffer[5:]
          waiting_for = 'body'
        elif waiting_for == 'body':
          self.handle_body(msg_ID, recv_buffer[:wait_for_num_bytes])
          recv_buffer = recv_buffer[wait_for_num_bytes:]
          waiting_for = 'header'
          wait_for_num_bytes = 5

  def deserialize_header(self, header_raw):
    return header_raw[0], int.from_bytes(header_raw[1:5], byteorder=sys.byteorder)

  def handle_body(self, msg_ID, body_raw):
    if msg_ID == 0:
      # Path to target executable (i.e. the executable that is emulated)
      # This message is sent first on every run
      body = body_raw.decode('utf-8')
      self.process_messages.initial_message(body)
    elif msg_ID == 1:
      # coverage event
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
      # crash event
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
      # fuzzer statistics event
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
