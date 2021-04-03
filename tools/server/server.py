#!/usr/bin/env python3

import socket
import multiprocessing as mp
import _thread
import threading
import subprocess
import argparse
import os
import shutil
import glob
import time
import signal


PORT = 8123
COVERAGE_DATA_DIR = 'coverage_files'
OLD_COVERAGE_DATA_DIR = 'previous_runs'


def signal_handler(sig, frame):
  print('\nExiting...')
  os._exit(0)
signal.signal(signal.SIGINT, signal_handler)


class Gcovr(threading.Thread):
  new_coverage = False

  def __init__(self, sleep_time = 2):
    threading.Thread.__init__(self)
    self.name = 'gcov_thread'
    self.sleep_time = sleep_time
    self.new_coverage = False

  def run(self):
    while True:
      if self.new_coverage:
        self.new_coverage = False
        self.run_gcovr()
      time.sleep(self.sleep_time)

  def run_gcovr(self):
    os.system(f'cd {COVERAGE_DATA_DIR} && gcovr -b  . -g -k --html --html-details -o coverage.html')


def listen_for_messages(recv_queue):
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(("127.0.0.1", PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
      print(f"Connection established with {addr}")
      while True:
        # wait for recv of header
        data = conn.recv(4096)
        if len(data) == 0:
          # TODO: tell the other thread to exit too
          print("Connection was closed")
          os._exit(0)
        recv_queue.put(data)


def addr_to_src(path_to_binary: str, addr: int) -> str:
  cmd = ['avr-addr2line', '-e', path_to_binary, hex(addr)]
  out = subprocess.check_output(cmd, timeout=10)
  return out


def update_coverage(args, gcovr, from_addr: int, to_addr: int):
  # looks more clear if we only mark the to_addr line
  src_location = addr_to_src(args.path_to_binary, to_addr)
  src_location = src_location.decode('UTF-8').strip()
  if src_location.startswith('??:'):
    return 
  # decode and remove trailing newline
  # if not key exists: mkdir -p of basedir and cp src. check if src exists
  if not os.path.isabs(src_location) or not src_location[0] == '/':
    assert False, f"Expected addr2line output {src_location} to be an absolute path"
  src_location_file, src_location_line = src_location.split(':')
  if not os.path.exists(src_location_file):
    return 
  cached_source_code_file = os.path.join(COVERAGE_DATA_DIR, src_location_file[1:])
  gcov_file = cached_source_code_file + '.gcov'
  if not os.path.exists(cached_source_code_file):
    os.makedirs(os.path.dirname(cached_source_code_file), exist_ok=True)
    shutil.copyfile(src_location_file, cached_source_code_file)
    with open(gcov_file, 'w') as f:
      # 'Source:' is relative to COVERAGE_DATA_DIR
      f.write(f'-:0:Source:{src_location_file[1:]}\n')
  with open(gcov_file, 'a') as f:
    f.write(f'1:{src_location_line}:\n')
  gcovr.new_coverage = True


def deserialize_header(header_raw):
  # TODO: byteoder depends on CPU. do we ever use big endian? and does it matter if its run in a VM?
  return header_raw[0], int.from_bytes(header_raw[1:5], byteorder='little')


def handle_body(args, gcovr, msg_ID, body_raw):
  if msg_ID == 1:
    # coverage event
    # body consists of: from addr (32 bit), to addr (32 bit)
    assert len(body_raw) == 8, f"ERROR: expected msg_body size 8, got {len(body_raw)}"
    from_addr = int.from_bytes(body_raw[:4], byteorder='little')
    to_addr = int.from_bytes(body_raw[4:8], byteorder='little')
    #print(f'{from_addr=} {addr_to_src(args.path_to_binary, from_addr)}, '
    #      f'{to_addr=} {addr_to_src(args.path_to_binary, to_addr)}')
    update_coverage(args, gcovr, from_addr, to_addr)
  else:
    assert False, f"unknown {msg_ID=}"


def read_from_queue(args, gcovr, msg_queue):
  recv_buffer = b''
  waiting_for = 'header'
  header_len = 5
  wait_for_num_bytes = header_len
  msg_ID = -1
  while True:
    msg = msg_queue.get()
    recv_buffer += msg
    while len(recv_buffer) > wait_for_num_bytes:
      if waiting_for == 'header':
        msg_ID, wait_for_num_bytes = deserialize_header(recv_buffer[:5])
        recv_buffer = recv_buffer[5:]
        waiting_for = 'body'
      elif waiting_for == 'body':
        handle_body(args, gcovr, msg_ID, recv_buffer[:wait_for_num_bytes])
        recv_buffer = recv_buffer[wait_for_num_bytes:]
        waiting_for = 'header'
        wait_for_num_bytes = 5


def move_old_data():
  if not os.path.exists(COVERAGE_DATA_DIR):
    os.mkdir(COVERAGE_DATA_DIR)
    return
  if not os.path.exists(OLD_COVERAGE_DATA_DIR):
    os.mkdir(OLD_COVERAGE_DATA_DIR)
  modified_epoch_time = os.path.getmtime(COVERAGE_DATA_DIR)
  modified_time = time.strftime('%d_%b_%Y_%H:%M:%S_%Z', time.localtime(modified_epoch_time))
  move_dir_to = os.path.join(OLD_COVERAGE_DATA_DIR, f'previous_run_from_{modified_time}')
  shutil.move(COVERAGE_DATA_DIR, move_dir_to)
  os.mkdir(COVERAGE_DATA_DIR)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="""\
    Receives messages from the emulator process, including coverage
    information, and presents this information in a human readable format.
  """)
  parser.add_argument('path_to_binary')
  parser.add_argument('--keep_coverage', action='store_true')
  args = parser.parse_args()
  if not args.keep_coverage:
    move_old_data()

  msg_queue = mp.Queue()
  #p = mp.Process(target=listen_for_messages, args=(msg_queue,))
  #p.start()
  # TODO: replace with multiprocessing if needed later
  _thread.start_new_thread(listen_for_messages, (msg_queue,))
  gcovr = Gcovr()
  gcovr.start()
  read_from_queue(args, gcovr, msg_queue)
