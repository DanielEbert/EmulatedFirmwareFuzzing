import multiprocessing as mp
from sl.process_messages import Process_Messages
import sys


class Parser:
  def __init__(self, client_packages: mp.Queue):
    self.client_packages = client_packages
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
      # path to target executable (i.e. the executable that is emulated)
      body = body_raw.decode('utf-8')
      self.process_messages.set_path_to_emulated_executable(body)
    elif msg_ID == 1:
      # coverage event
      # body consists of: from addr (32 bit), to addr (32 bit)
      assert len(
          body_raw) == 8, f"ERROR: expected msg_body size 8, got {len(body_raw)}"
      from_addr = int.from_bytes(body_raw[:4], byteorder='little')
      to_addr = int.from_bytes(body_raw[4:8], byteorder='little')
      # print(f'{from_addr=} {addr_to_src(args.path_to_binary, from_addr)}, '
      #      f'{to_addr=} {addr_to_src(args.path_to_binary, to_addr)}')
      self.process_messages.update_coverage(from_addr, to_addr)
    elif msg_ID == 2:
      crash_ID = int(body_raw[0])
      crashing_addr = int.from_bytes(body_raw[1:5], byteorder='little')
      crashing_input = body_raw[5:]
      self.process_messages.save_crashing_input(
          crash_ID, crashing_addr, crashing_input)
    else:
      assert False, f"unknown {msg_ID=}"
