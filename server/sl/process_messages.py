from sl.update_ui import Update_UI
import subprocess
import os
import shutil


# TODO: refactor. We also have these constants in update_ui
CURRENT_RUN_DIR = '/home/user/EFF/current_run'
CRASHING_INPUTS_DIR = '/home/user/EFF/current_run/crashing_inputs'


class Process_Messages:
  def __init__(self):
    self.update_ui = Update_UI()
    self.update_ui.start()
    self.path_to_emulated_executable = None

  def set_path_to_emulated_executable(self, path):
    self.path_to_emulated_executable = path

  def update_coverage(self, from_addr: int, to_addr: int):
    if self.path_to_emulated_executable is None:
      print("Warning: Client has not sent path_to_emulated_executable yet.")
      return
    # looks more clear if we only mark the to_addr line
    src_location = self.addr_to_src(self.path_to_emulated_executable, to_addr)
    src_location = src_location.decode('UTF-8').strip()
    if src_location.startswith('??:'):
      return
    # decode and remove trailing newline
    # if not key exists: mkdir -p of basedir and cp src. check if src exists
    if not os.path.isabs(src_location) or not src_location[0] == '/':
      assert False, f"Expected addr2line output {src_location} to be an absolute path"
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

  def save_crashing_input(self, crash_ID: int, crashing_addr: int, inp: bytes):
    prefix = ''
    if crash_ID == 1:
      prefix = 'stack_buffer_overflow'  # TODO: prefix is vaddr
    else:
      assert False, f'Unknown crashing input ID {crash_ID}'
    # TODO: print addr in hex
    filename = f'{prefix}_{crashing_addr:x}'
    file_path = os.path.join(CRASHING_INPUTS_DIR, filename)
    if os.path.exists(file_path):
      # print(f'Skipping duplicate crashing input file: {filename}')
      return
    with open(file_path, 'wb') as f:
      f.write(inp)

  def addr_to_src(self, path_to_binary: str, addr: int) -> str:
    cmd = ['avr-addr2line', '-e', path_to_binary, hex(addr)]
    out = subprocess.check_output(cmd, timeout=10)
    return out
