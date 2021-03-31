#!/usr/bin/env python3

import socket
import multiprocessing as mp
from time import sleep
import _thread
import subprocess
import argparse



PORT = 8123


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
          exit(0)
        recv_queue.put(data)


def addr_to_src(path_to_binary: str, addr: int) -> str:
  cmd = ['avr-addr2line', '-e', path_to_binary, str(addr)]
  out = subprocess.check_output(cmd, timeout=10)
  return out

def deserialize_header(header_raw):
  # TODO: byteoder depends on CPU. do we ever use big endian? and does it matter if its run in a VM?
  return header_raw[0], int.from_bytes(header_raw[1:5], byteorder='little')


def handle_body(args, msg_ID, body_raw):
  if msg_ID == 1:
    # coverage event
    # body is: from addr (32 bit), to addr (32 bit)
    assert len(body_raw) == 8, f"ERROR: expected msg_body size 8, got {len(body_raw)}"
    from_addr = int.from_bytes(body_raw[:4], byteorder='little')
    to_addr = int.from_bytes(body_raw[4:8], byteorder='little')
    print(f'{from_addr=} {addr_to_src(args.path_to_binary, from_addr)}, '
          f'{to_addr=} {addr_to_src(args.path_to_binary, to_addr)}')
  else:
    assert False, f"unknown {msg_ID=}"


def read_from_queue(args, msg_queue):
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
        handle_body(args, msg_ID, recv_buffer[:wait_for_num_bytes])
        recv_buffer = recv_buffer[wait_for_num_bytes:]
        waiting_for = 'header'
        wait_for_num_bytes = 5


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="""\
    Recieves messages from the emulator, including coverage information, and
    presents this information in a human readable format.
  """)
  parser.add_argument('path_to_binary')

  args = parser.parse_args()

  msg_queue = mp.Queue()
  #p = mp.Process(target=listen_for_messages, args=(msg_queue,))
  #p.start()
  # TODO: replace with multiprocessing if needed later
  _thread.start_new_thread(listen_for_messages, (msg_queue,))
  try:
    read_from_queue(args, msg_queue)
  finally:
    #p.join()
    pass
