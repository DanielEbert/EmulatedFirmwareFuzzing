#!/usr/bin/env python3

import os
import subprocess

cwd = os.getcwd()
# TODO: assert here?

test_env = os.environ.copy()
test_env['LD_PRELOAD'] = '../../simavr/simavr/sim/patches/reset_on_loop_call.c.so'

# TODO: on remote i dont have mutator. doesnt matter for now
default_flags = ['--run_once_with', 'input', '--mutator_so_path', '/home/user/EFF/simavr/simavr/mutators/libfuzzer/libfuzzer-mutator.so']

tests = [
  (
    'uninitialized7', b'uninitialized memory found', default_flags
  ),
  (
    'stack_buffer_overflow', b'Stack Smashing Detected', default_flags
  ),
  (
    'stack_buffer_overflow_after_ret', b'Stack Smashing Detected', default_flags
  ),
  (
    'uninitialized6', b'uninitialized memory found', default_flags
  ),
  (
    'uninitialized2', b'uninitialized memory found', default_flags
  ),
  (
    'uninitialized', b'uninitialized memory found', default_flags
  ),
  (
    'timeout', b'Timeout found', default_flags + ['--timeout', '1300']
  ),
]

for test_dir, required_msg, flags in tests:
  os.chdir(test_dir)
  os.system("echo 'A' > input")
  exit_code = os.system('make --silent')
  assert exit_code == 0, f'make returned exit code {exit_code}'
  print(['../../simavr/simavr/run_avr' ,'-m', 'atmega2560'] + flags + ['src.ino.elf'])
  p = subprocess.run(['../../simavr/simavr/run_avr' ,'-m', 'atmega2560'] + flags + ['src.ino.elf'], stdout = subprocess.PIPE, stderr=subprocess.PIPE, env=test_env)
  assert required_msg in p.stdout, f'Test {test_dir} FAILED\nstdout for test {test_dir} did not include the required text: {required_msg}\nstdout was:\n{p.stdout}'
  print(f'***** Test {test_dir} SUCCESS')
  os.chdir(cwd)
