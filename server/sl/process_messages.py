from sl.update_ui import Update_UI
import subprocess
import os
import shutil
import hashlib
import time


# TODOE: maybe refactor
CURRENT_RUN_DIR = '/home/user/EFF/current_run'
CRASHING_INPUTS_DIR = '/home/user/EFF/current_run/crashing_inputs'
PREVIOUS_INTERESTING_INPUTS_DIR = '/home/user/EFF/current_run/previous_interesting_inputs'
OLD_RUNS_DIR = '/home/user/EFF/previous_runs'


class Fuzzer_Stats:
  def __init__(self):
    self.fuzzer_start_time = None
    self.inputs_executed = 0
    self.previous_interesting_inputs = 0
    self.edges_found = 0
    self.total_crashes = 0
    self.max_depth = 0
    self.uninitialized_value_used_count = 0
    self.timeout_count = 0
    self.invalid_write_address_count = 0
    self.bad_jump_count = 0
    self.reading_past_end_of_flash_count = 0
    self.stack_buffer_overfow_count = 0
    self.stats_update_time = None
    # average of the last X seconds
    self.average_inputs_executed_last_10_seconds = 0


class Process_Messages:
  def __init__(self):
    self.fuzzer_stats = Fuzzer_Stats()
    self.move_old_data()
    self.update_ui = Update_UI(self, CURRENT_RUN_DIR)
    self.update_ui.start()
    self.path_to_emulated_executable = None

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

  def update_coverage(self, from_addr: int, to_addr: int):
    if self.path_to_emulated_executable is None:
      print("Warning: Client has not sent path_to_emulated_executable yet.")
      return
    self.fuzzer_stats.edges_found += 1
    # looks more clear if we only mark the to_addr line
    src_location = self.addr_to_src(self.path_to_emulated_executable, to_addr)
    src_location = src_location.decode('UTF-8').strip()
    if src_location.startswith('??:'):
      return
    # decode and remove trailing newline
    # if not key exists: mkdir -p of basedir and cp src. check if src exists
    if not os.path.isabs(src_location) or not src_location[0] == '/':
      print(f'no absolute path {src_location}')
      return
    src_location_file, src_location_line = src_location.split(':')
    if not os.path.exists(src_location_file):
      return
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

  def save_previous_interesting_input(self, inp: bytes):
    # sha1 as filename
    filename = hashlib.sha1(inp).hexdigest()
    file_path = os.path.join(PREVIOUS_INTERESTING_INPUTS_DIR, filename)
    if os.path.exists(file_path):
      # print(f'Skipping duplicate crashing input file: {filename}')
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
      # print(f'Skipping duplicate crashing input file: {filename}')
      return
    with open(file_path, 'wb') as f:
      f.write(inp)
    file_path += '_info'
    with open(file_path, 'w') as f:
      f.write(stack_frame_text)

  def update_fuzzer_stats(self, inputs_executed, total_crashes, max_depth):
    assert self.fuzzer_stats.fuzzer_start_time != None
    assert self.fuzzer_stats.stats_update_time != None
    assert self.fuzzer_stats.inputs_executed <= inputs_executed
    current_time = time.time()
    assert current_time > self.fuzzer_stats.stats_update_time
    time_delta = current_time - self.fuzzer_stats.stats_update_time
    executed_inputs_delta = inputs_executed - self.fuzzer_stats.inputs_executed
    self.fuzzer_stats.average_inputs_executed_last_10_seconds = executed_inputs_delta / time_delta

    self.fuzzer_stats.stats_update_time = current_time
    self.fuzzer_stats.inputs_executed = inputs_executed
    self.fuzzer_stats.total_crashes = total_crashes
    self.fuzzer_stats.max_depth = max_depth

  def addr_to_src(self, path_to_binary: str, addr: int, function_name=False) -> str:
    cmd = ['avr-addr2line', '-e', path_to_binary, hex(addr)]
    if function_name:
      cmd += ['--functions', '--pretty-print']
    out = subprocess.check_output(cmd, timeout=10)
    return out
