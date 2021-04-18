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
]

for test_dir, required_msg in tests:
  os.chdir(test_dir)
  exit_code = os.system('make --silent')
  assert exit_code == 0, f'make returned exit code {exit_code}'
  # need to check stderr
  p = subprocess.run(['../../simavr/simavr/run_avr' ,'-m', 'atmega2560', '--run_once_with', 'Makefile', 'src.ino.elf'], stdout = subprocess.PIPE, stderr=subprocess.PIPE, env=test_env)
  assert required_msg in p.stdout, f'Test {test_dir} FAILED\nstderr for test {test_dir} did not include the required text: {required_msg}'
  print(f'***** Test {test_dir} SUCCESS')
  os.chdir(cwd)
