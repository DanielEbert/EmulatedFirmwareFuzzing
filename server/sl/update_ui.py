import threading
import time
import os
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import shutil
import datetime


class Update_UI(threading.Thread):
  start_time = None
  new_coverage = False
  # list of tuples: (epoch, edges reached)
  edges_plot_data = []

  def __init__(self, process_messages, CURRENT_RUN_DIR, sleep_time=5):
    threading.Thread.__init__(self)
    self.name = 'UI_thread'
    self.process_messages = process_messages
    self.sleep_time = sleep_time
    self.new_coverage = False
    self.CURRENT_RUN_DIR = CURRENT_RUN_DIR

  def run(self):
    while True:
      self.write_fuzzer_stats()
      if self.new_coverage:
        self.new_coverage = False
        self.run_gcovr()
      # always update coverage plot
      self.plot_coverage()
      time.sleep(self.sleep_time)

  def write_fuzzer_stats(self):
    fuzzer_stats_file_path = os.path.join(self.CURRENT_RUN_DIR, 'fuzzer_stats')
    with open(fuzzer_stats_file_path, 'w') as f:
      f.write('Date: {:%Y-%m-%d %H:%M:%S}\n'.format(datetime.datetime.now()))
      for stat in [i for i in dir(self.process_messages.fuzzer_stats)
                   if not i.startswith('__')]:
        f.write(f'{stat}: {vars(self.process_messages.fuzzer_stats)[stat]}, ')
      f.write('\n')

  def on_new_edge(self):
    self.new_coverage = True
    if self.start_time is None:
      self.start_time = int(time.time())
    self.edges_plot_data.append(
        (time.time() - self.start_time, len(self.edges_plot_data) + 1))

  def run_gcovr(self):
    os.system(
        f'cd {self.CURRENT_RUN_DIR} && gcovr -b  . -g -k --html --html-details -o coverage.html')

  def plot_coverage(self):
    if not self.edges_plot_data:
      return
    data = self.edges_plot_data + \
        [(int(time.time() - self.start_time), len(self.edges_plot_data))]
    frame = pd.DataFrame(data, columns=['Time (seconds)', 'Edges Reached'])
    plot = sns.lineplot(data=frame, x='Time (seconds)', y='Edges Reached')
    plot.set(xscale='log')
    plot.figure.savefig(f'{self.CURRENT_RUN_DIR}/coverage_over_time_plot.png')
    plt.clf()  # clear plot
