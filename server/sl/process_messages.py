from sl.update_ui import Update_UI
import subprocess
import os
import shutil
import hashlib
import time
import amoco
from amoco.arch.core import type_control_flow 

CURRENT_RUN_DIR = '/home/user/EFF/current_run'
CRASHING_INPUTS_DIR = '/home/user/EFF/current_run/crashing_inputs'
PREVIOUS_INTERESTING_INPUTS_DIR = '/home/user/EFF/current_run/previous_interesting_inputs'
OLD_RUNS_DIR = '/home/user/EFF/previous_runs'


class Fuzzer_Stats:
  def __init__(self):
    # Time in seconds since the epoch
    self.fuzzer_start_time = None
    # Total number of inputs executed
    self.inputs_executed = 0
    # How many inputs have increased the coverage
    self.previous_interesting_inputs = 0
    # How many unique <source address, destionation address> pairs of jumps
    # and calls were found.
    self.edges_found = 0
    # Number of unique and non-unique crashes, i.e. total crashes
    self.total_crashes = 0
    # The number of stack frames in the largest stack trace observed so far.
    self.max_depth = 0
    # The following *_count variable specify how many unique crashes of the
    # respective crash type have been found.
    self.uninitialized_value_used_count = 0
    self.timeout_count = 0
    self.invalid_write_address_count = 0
    self.bad_jump_count = 0
    self.reading_past_end_of_flash_count = 0
    self.stack_buffer_overfow_count = 0
    # Epoch time when the last 'Updated Fuzzer Statistics' message was received 
    self.stats_update_time = None
    # average total number of inputs executed in the last ~30 seconds
    self.inputs_executed_per_second = 0


class Process_Messages:
  def __init__(self):
    self.fuzzer_stats = Fuzzer_Stats()
    self.update_ui = Update_UI(self, CURRENT_RUN_DIR)
    self.path_to_emulated_executable = None
    self.disassembler = None

  # If the current_run directory exists, move the current_run directory into 
  # the previous_runs/ directory.
  def move_old_data(self):
    if not os.path.exists(CURRENT_RUN_DIR):
      os.mkdir(CURRENT_RUN_DIR)
      os.mkdir(CRASHING_INPUTS_DIR)
      os.mkdir(PREVIOUS_INTERESTING_INPUTS_DIR)
      return
    if not os.path.exists(OLD_RUNS_DIR):
      os.mkdir(OLD_RUNS_DIR)
    modified_epoch_time = os.path.getmtime(CURRENT_RUN_DIR)
    modified_time = time.strftime(
        '%d_%b_%Y_%H:%M:%S_%Z', time.localtime(modified_epoch_time))
    move_dir_to = os.path.join(
        OLD_RUNS_DIR, f'previous_run_from_{modified_time}')
    shutil.move(CURRENT_RUN_DIR, move_dir_to)
    os.mkdir(CURRENT_RUN_DIR)
    os.mkdir(CRASHING_INPUTS_DIR)
    os.mkdir(PREVIOUS_INTERESTING_INPUTS_DIR)

  def initial_message(self, path):
    self.fuzzer_stats.fuzzer_start_time = time.time()
    self.fuzzer_stats.stats_update_time = time.time()
    self.path_to_emulated_executable = path
    self.move_old_data()
    self.update_ui.start()
    try:
      prog = amoco.load_program(path)
      # Disassembler uses the 'Linear Disassembly' strategie, which works
      # well for our use case. (Linear Disassembly does not work well if
      # instructions and data is mixed. In this use case this is not the case.)
      self.disassembler = amoco.sa.lsweep(prog)
    except Exception as e:
      print(f'Disassembly of file {path} failed. Exception:', e)
      self.disassembler = None

  def update_coverage(self, to_addr: int):
    self.fuzzer_stats.edges_found += 1
    if self.path_to_emulated_executable is None:
      print("Warning: Client has not sent path_to_emulated_executable yet.")
      return
    if self.disassembler is None:
      print(f"Warning: Disassemby of {self.path_to_emulated_executable} failed.")
      return
    # The 'instructions' list stores the instructions from address 'to_addr' to 
    # to next RETURN instruction
    instructions = []
    disass = self.disassembler.sequence(to_addr)
    while True:
      # Iterate until we find a RETURN instruction
      try:
        instr = next(disass)
      except Exception as e:
        break
      instructions.append(instr)
      if instr.type == type_control_flow and 'CALL' not in instr.mnemonic:
        break

    # Get the source code locations of each instruction in 'instructions'
    # Write this location in one of the files that are later passed to gcovr.
    # Writes are formatted correctly for gcovr.
    for i in instructions:
      addr = i.address.value
      src_location = self.addr_to_src(self.path_to_emulated_executable, addr)
      src_location = src_location.decode('UTF-8').strip()
      if src_location.startswith('??:'):
         break
      # decode and remove trailing newline
      # if not key exists: mkdir -p of basedir and cp src. check if src exists
      if not os.path.isabs(src_location) or not src_location[0] == '/':
        print(f'no absolute path {src_location}')
        break
      src_location_file, src_location_line = src_location.split(':')
      if not os.path.exists(src_location_file):
        break
      cached_source_code_file = os.path.join(
          CURRENT_RUN_DIR, src_location_file[1:])
      gcov_file = cached_source_code_file + '.gcov'
      if not os.path.exists(cached_source_code_file):
        os.makedirs(os.path.dirname(cached_source_code_file), exist_ok=True)
        shutil.copyfile(src_location_file, cached_source_code_file)
        with open(gcov_file, 'w') as f:
          # 'Source:' is relative to CURRENT_RUN_DIR
          f.write(f'-:0:Source:{src_location_file[1:]}\n')
      with open(gcov_file, 'a') as f:
        f.write(f'1:{src_location_line}:\n')
    self.update_ui.on_new_edge()

  # Store 'inp' in a file.
  def save_previous_interesting_input(self, inp: bytes):
    # sha1 of inp is used as filename
    filename = hashlib.sha1(inp).hexdigest()
    file_path = os.path.join(PREVIOUS_INTERESTING_INPUTS_DIR, filename)
    if os.path.exists(file_path):
      # Skip duplicates (i.e. if a file with the same 'inp' was stored already) 
      return
    self.fuzzer_stats.previous_interesting_inputs += 1
    with open(file_path, 'wb') as f:
      f.write(inp)

  def crashing_input(self, crash_ID, crash_addr, origin_addr, crash_input, stack_frames_pc):
    # Found a new crash, write the stacktrace and the crashing input to a file
    stack_frame_text = 'Stackframe at crash:\n'
    for i, pc in enumerate(stack_frames_pc):
      stack_frame_text += f'    #{i} 0x{pc:x} in '
      stack_frame_text += self.addr_to_src(
          self.path_to_emulated_executable, pc, function_name=True
      ).decode('UTF-8').strip()
      stack_frame_text += '\n'

    self.save_crashing_input(
        crash_ID, crash_addr, origin_addr, crash_input, stack_frame_text)

  # The name of the file where the crashing input is stored consists of the
  # type of crash and when (i.e. at what program counter) the crash was
  # first noticed. In case of a UUM, the origin address is also part of the
  # filename.
  def save_crashing_input(self, crash_ID: int, crash_addr: int, origin_addr: int,
                          inp: bytes, stack_frame_text: str):
    prefix = ''
    if crash_ID == 0:
      self.fuzzer_stats.stack_buffer_overfow_count += 1
      prefix = 'stack_buffer_overflow'
      filename = f'{prefix}_{crash_addr:x}'
    elif crash_ID == 1:
      self.fuzzer_stats.uninitialized_value_used_count += 1
      prefix = 'uninitialized_value_used_at'
      filename = f'{prefix}_{crash_addr:x}_with_origin_{origin_addr:x}'
    elif crash_ID == 2:
      self.fuzzer_stats.timeout_count += 1
      prefix = 'timeout'
      filename = f'{prefix}_{crash_addr:x}'
    elif crash_ID == 3:
      self.fuzzer_stats.invalid_write_address_count += 1
      prefix = 'invalid_write_address'
      filename = f'{prefix}_{crash_addr:x}'
    elif crash_ID == 4:
      self.fuzzer_stats.bad_jump_count += 1
      prefix = 'bad_jump_found'
      filename = f'{prefix}_{crash_addr:x}'
    elif crash_ID == 5:
      self.fuzzer_stats.reading_past_end_of_flash_count += 1
      prefix = 'reading_past_end_of_flash'
      filename = f'{prefix}_{crash_addr:x}'
    else:
      assert False, f'Unknown crashing input ID {crash_ID}'
    file_path = os.path.join(CRASHING_INPUTS_DIR, filename)
    if os.path.exists(file_path):
      return
    with open(file_path, 'wb') as f:
      f.write(inp)
    file_path += '_info'
    with open(file_path, 'w') as f:
      f.write(stack_frame_text)

  # Updates the class variables in the Fuzzer_Stats class.
  def update_fuzzer_stats(self, inputs_executed, total_crashes, max_depth):
    assert self.fuzzer_stats.fuzzer_start_time != None
    assert self.fuzzer_stats.stats_update_time != None
    assert self.fuzzer_stats.inputs_executed <= inputs_executed
    current_time = time.time()
    assert current_time > self.fuzzer_stats.stats_update_time
    time_delta = current_time - self.fuzzer_stats.stats_update_time
    executed_inputs_delta = inputs_executed - self.fuzzer_stats.inputs_executed
    self.fuzzer_stats.inputs_executed_per_second = executed_inputs_delta / time_delta

    self.fuzzer_stats.stats_update_time = current_time
    self.fuzzer_stats.inputs_executed = inputs_executed
    self.fuzzer_stats.total_crashes = total_crashes
    self.fuzzer_stats.max_depth = max_depth

  # Returns the source code location of the instruction at address 'addr' in the
  # address space of the emulated program 'patch_to_emulated_executable'
  def addr_to_src(self, path_to_emulated_executable: str, addr: int, function_name=False) -> str:
    cmd = ['avr-addr2line', '-e', path_to_emulated_executable, hex(addr)]
    if function_name:
      cmd += ['--functions', '--pretty-print']
    out = subprocess.check_output(cmd, timeout=10)
    return out
