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
    p = mp.Process(target=self.listen_for_packages)
    p.start()
    self.parser = Parser(self.client_packages)

  def listen_for_packages(self):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
      sock.bind(("127.0.0.1", self.listen_port))
      sock.listen()
      conn, addr = sock.accept()
      with conn:
        print(f"Connection established with {addr}")
        while True:
          # wait for recv of header
          data = conn.recv(4096)
          if len(data) == 0:
            print("Connection was closed")
            sock.close()
            # Kill Parent
            parent_pid = os.getppid()
            if parent_pid != 1:
              os.kill(parent_pid, signal.SIGTERM)
            os._exit(0)
          self.client_packages.put(data)
    finally:
      sock.close()
