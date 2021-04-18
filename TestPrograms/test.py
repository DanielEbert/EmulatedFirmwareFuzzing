#!/usr/bin/env python3

import os
import subprocess

cwd = os.getcwd()
# TODO: assert here?

test_env = os.environ.copy()
test_env['LD_PRELOAD'] = '../../simavr/simavr/sim/patches/reset_on_loop_call.c.so'

tests = [
  (
    'uninitialized7', b'''\
SF not set at pc 6a2 with origin 6d0
SF not set at pc 6dc with origin 6d0
Exiting normally.
'''
  ),
  (
    'stack_buffer_overflow', b'''\
06a8 : Stack Smashing Detected
SP 21ed, A=21f7 <= 41
Exiting normally.
'''
  ),
  (
    'uninitialized6', b'''\
SF not set at pc 558 with origin 79a
Exiting normally.
'''
  ),
  (
    'uninitialized4', b'''\
SF not set at pc 65c with origin 64a
Exiting normally.
'''
  ),
  (
    'uninitialized2', b'''\
SF not set at pc 7c8 with origin 7c0
Exiting normally.
'''
  ),
  (
    'uninitialized', b'''\
SF not set at pc 7a0 with origin 798
SF not set at pc 990 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9ae with origin 7b0
SF not set at pc 9b6 with origin 7b0
SF not set at pc 9bc with origin 7b0
SF not set at pc 9c4 with origin 7b0
Exiting normally.
'''
  ),
]

for test_dir, required_msg in tests:
  os.chdir(test_dir)
  exit_code = os.system('make --silent')
  assert exit_code == 0, f'make returned exit code {exit_code}'
  p = subprocess.run(['../../simavr/simavr/run_avr' ,'-m', 'atmega2560', '--run_once_with', 'Makefile', 'src.ino.elf'], stdout = subprocess.PIPE, stderr=subprocess.PIPE, env=test_env)
  assert required_msg in p.stdout, f'Test {test_dir} FAILED\nstdout for test {test_dir} did not include the required text: {required_msg}\nstdout was:\n{p.stdout}'
  print(f'***** Test {test_dir} SUCCESS')
  os.chdir(cwd)
