"""
This script summarizes the Microsoft-Windows-Win32k events ExclusiveUserCrit,
SharedUserCrit, and ReleaseUserCrit to understand how frequently this lock is
acquired and released and how long it is held and waited for.

Be aware that it appears that the events are *not* emitted in many cases, for
reasons that are not clear.

This script defaults to giving details of three particular processes and
summarizes the others. The names are hardcoded.

Prior to running this test you need to export the events to a .csv file with
wpaexporter. See export.bat for examples.

This script assumes a QPC frequency of 10 MHz. To avoid this assumption would
require extracting the QPC frequency from the ETW trace.

See this post and its successor for details:
https://randomascii.wordpress.com/2018/12/03/a-not-called-function-can-cause-a-5x-slowdown/
"""

import sys

if len(sys.argv) != 2:
  print "Usage: %s csv_filename" % sys.argv[0]
  sys.exit(0)
filename = sys.argv[1]

# Remap from ETW event names to appropriate names to use in the report, complete
# with merging event names to a single label.
acquire_label = "Wait time"
release_label = "Held time"
event_remap = {"ExclusiveUserCrit" : acquire_label, "SharedUserCrit" :
               acquire_label, "ReleaseUserCrit" : release_label}

interesting_processes = ['conhost.exe', 'processcreatetests.exe', 'unknown']

# True on my laptop...
qpc_frequency = 1e7

# File this in as: processes[process_name][event_data] = QPC_count_list
processes = {}

# Skip the line with the headings.
for line in open(filename).readlines()[1:]:
  # Split the line into event name, process name (with PID), and QPC count
  # Note that this will fail if the .csv file has any embedded commas.
  event, process, QPC_count = line.strip().split(',')
  # Rename the events.
  event = event_remap[event]
  # Remove the PID from the process name.
  process_name = process.split()[0]
  # Accumulate the data into a dictionary of dictionaries of lists.
  # That is: processes[process_name][event_data] = QPC_count_list
  process_data = processes.get(process_name, {})
  processes[process_name] = process_data
  event_data = process_data.get(event, [])
  process_data[event] = event_data
  event_data.append(int(QPC_count))

process_names = processes.keys()
process_names.sort() # Consistent ordering is nice.
# Summarize by process names.
total_sum = {}
total_count = {}
for process_name in process_names:
  process = processes[process_name]
  # Trying to generically identify 'interesting' processes
  # works 'okay', but not as well for my purposes.
  #print_details = len(process[process.keys()[0]]) > 2000
  print_details = process_name.lower() in interesting_processes
  if print_details:
    print('Process %s:' % process_name)
  for event_name in process.keys():
    event_data = process[event_name]
    sum = 0
    for datum in event_data:
      sum += datum
    total_sum[event_name] = total_sum.get(event_name, 0) + sum
    total_count[event_name] = total_count.get(event_name, 0) + len(event_data)
    if print_details:
      print('  %s: %7.3f s, %6d events' % (event_name, sum / qpc_frequency,
                                           len(event_data)))

print('Totals:')
for event_name in total_sum.keys():
  print('  %s: %7.3f s, %6d events' % (event_name, total_sum[event_name] /
                                       qpc_frequency, total_count[event_name]))
