#!/usr/bin/env python3

import os
import subprocess

cwd = os.getcwd()
# TODO: assert here?

test_env = os.environ.copy()
test_env['LD_PRELOAD'] = '../../simavr/simavr/sim/patches/reset_on_loop_call.c.so'

tests = [
  (
    'uninitialized7', b'SF flags not set'
  ),
  (
    'stack_buffer_overflow', b'Stack Smashing Detected'
  ),
  (
    'uninitialized6', b'SF flags not set'
  ),
  (
    'uninitialized4', b'SF flags not set'
  ),
  (
    'uninitialized2', b'SF flags not set'
  ),
  (
    'uninitialized', b'SF flags not set'
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
