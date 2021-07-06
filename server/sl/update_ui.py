import threading
import time
import os
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import shutil
import datetime


class Update_UI(threading.Thread):

  def __init__(self, process_messages, CURRENT_RUN_DIR, sleep_time=5):
    threading.Thread.__init__(self)
    self.name = 'UI_thread'
    self.process_messages = process_messages
    # The UI (e.g. gcovr's HTML pages) are updated every sleep_time seconds
    # If the fuzzing run just started, the fuzzer typically finds many edges in
    # a short time. If we would call gcovr after every new edge, it would take
    # hours until gcovr has processed the edges of the first couple of seconds.
    # So instead, we don't run gcovr on every new edge found, but only if an
    # edge was found and 5 seconds since the last gcovr run have passed.
    self.sleep_time = sleep_time
    # Has a new edge been found? This is used to only call gcovr if the fuzzer 
    # has found new coverage since the last gcovr run.
    self.new_coverage = False
    self.CURRENT_RUN_DIR = CURRENT_RUN_DIR
    # Epoch time when the first edge was processed.
    self.start_time = None
    # list of tuples, where each tuple consists of: 
    # (epoch time, number of edges reached)
    # This is used for the edge coverage over time graph
    self.edges_plot_data = []

  def run(self):
    while True:
      self.write_fuzzer_stats()
      if self.new_coverage:
        self.new_coverage = False
        self.run_gcovr()
      # always update coverage plot
      self.plot_coverage()
      time.sleep(self.sleep_time)

  # Updates the 'current_run/fuzer_stats' file
  def write_fuzzer_stats(self):
    fuzzer_stats_file_path = os.path.join(self.CURRENT_RUN_DIR, 'fuzzer_stats')
    with open(fuzzer_stats_file_path, 'w') as f:
      f.write('Date: {:%Y-%m-%d %H:%M:%S}\n'.format(datetime.datetime.now()))
      for stat in [i for i in dir(self.process_messages.fuzzer_stats)
                   if not i.startswith('__')]:

        value = vars(self.process_messages.fuzzer_stats)[stat]
        if stat.endswith('_time'):
          formatted_value = time.strftime(
              "%a, %d %b %Y %H:%M:%S %Z", time.localtime(value))
          f.write(f'{stat}: {formatted_value}\n')
        else:
          f.write(f'{stat}: {value}\n')
      f.write('\n')

  def on_new_edge(self):
    self.new_coverage = True
    if self.start_time is None:
      self.start_time = int(time.time())
    self.edges_plot_data.append(
        (time.time() - self.start_time, len(self.edges_plot_data) + 1))

  def run_gcovr(self):
    # '-b' specifies that branch coverage is used
    # '.' specifies the 'CURRENT_RUN_DIR' as the search path for gcovr files 
    # '-g' specifies that gcov files are the input for gcovr
    # '-k' for keep, specifies that the gcov files are not deleted after
    #      gcovr has read them.
    # gcovr is a third-party library
    os.system(
        f'cd {self.CURRENT_RUN_DIR} && gcovr -b  . -g -k --html --html-details -o coverage.html')

  # Plot the edge coverage over time graph. This function uses the sns (seaborn)
  # third-party libary.
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
