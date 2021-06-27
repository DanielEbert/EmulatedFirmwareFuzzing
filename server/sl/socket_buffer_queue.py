import multiprocessing as mp
import socket
import os
from sl.parser import Parser
import signal


class Socket_Buffer_Queue:
  def __init__(self, listen_port=8123):
    self.listen_port = listen_port
    self.client_packages = mp.Queue()
    # Asynchronous waiting for packets. Packets are appended to the queue.
    parser = Parser(self.client_packages)
    parser.start()
    self.listen_for_packages()
    # If connection was closed: Kill child process
    parser.kill()
    parser.join()
    exit(0)

  def listen_for_packages(self):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("127.0.0.1", self.listen_port))
    sock.listen(0)
    conn, addr = sock.accept()
    with conn:
      print(f"Connection established with {addr}")
      while True:
        # wait for recv of header
        data = conn.recv(4096)
        if len(data) == 0:
          print("Connection was closed")
          sock.close()
          return
        self.client_packages.put(data)