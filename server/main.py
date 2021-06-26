#!/usr/bin/env python3

from sl.socket_buffer_queue import Socket_Buffer_Queue
import os
import signal
import multiprocessing as mp


def signal_handler(sig, frame):
  # On <CTRL> + C, kill all threads instead of one thread
  print('\nExiting...')
  os._exit(0)

signal.signal(signal.SIGINT, signal_handler)


def run():
  socket_buffer_queue = Socket_Buffer_Queue()


if __name__ == "__main__":
  while True: 
    p = mp.Process(target=run)
    p.start()
    p.join()
