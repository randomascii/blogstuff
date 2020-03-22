"""
This script finds the source of every compiled build target, the compile time,
the number of lines in the source file, the number of included dependencies,
and the number of lines in the dependencies.

To get clean results this should be done after a clean build (gn clean then
a build).

Redirect the output to a .csv file for rendering with d3 or analysis with
count_costs.py.
"""

from __future__ import print_function

import os
import sys


def SearchPath(exe_name):
  """Find an executable (.exe, .bat, whatever) in the system path."""
  for dir in os.environ['path'].split(';'):
    path = os.path.join(dir, exe_name)
    if os.path.exists(path):
      return path


class Target:
    """Represents a single line read for a .ninja_log file."""
    def __init__(self, start, end):
        """Creates a target object by passing in the start/end times in seconds
        as a float."""
        self.start = start
        self.end = end
        # A list of targets, appended to by the owner of this object.
        self.targets = []

    def Duration(self):
        """Returns the task duration in seconds as a float."""
        return self.end - self.start


# Copied with some modifications from ninjatracing
def ReadTargets(log, show_all):
    """Reads all targets from .ninja_log file |log_file|, sorted by duration.

    The result is a list of Target objects."""
    header = log.readline()
    assert header == '# ninja log v5\n', \
           'unrecognized ninja log version %r' % header
    targets_dict = {}
    last_end_seen = 0.0
    for line in log:
        parts = line.strip().split('\t')
        if len(parts) != 5:
          # If ninja.exe is rudely halted then the .ninja_log file may be
          # corrupt. Silently continue.
          continue
        start, end, _, name, cmdhash = parts # Ignore restat.
        # Convert from integral milliseconds to float seconds.
        start = int(start) / 1000.0
        end = int(end) / 1000.0
        if not show_all and end < last_end_seen:
            # An earlier time stamp means that this step is the first in a new
            # build, possibly an incremental build. Throw away the previous
            # data so that this new build will be displayed independently.
            # This has to be done by comparing end times because records are
            # written to the .ninja_log file when commands complete, so end
            # times are guaranteed to be in order, but start times are not.
            targets_dict = {}
        target = None
        if cmdhash in targets_dict:
          target = targets_dict[cmdhash]
          if not show_all and (target.start != start or target.end != end):
            # If several builds in a row just run one or two build steps then
            # the end times may not go backwards so the last build may not be
            # detected as such. However in many cases there will be a build step
            # repeated in the two builds and the changed start/stop points for
            # that command, identified by the hash, can be used to detect and
            # reset the target dictionary.
            targets_dict = {}
            target = None
        if not target:
          targets_dict[cmdhash] = target = Target(start, end)
        last_end_seen = end
        target.targets.append(name)
    return targets_dict.values()


def GetLineCount(line_counts, filename):
  """Find the length of a file. Results are cached in the line_counts
  dictionary."""
  if filename in line_counts:
    return line_counts[filename]
  with open(filename, 'r') as f:
    line_count = len(f.readlines())
  line_counts[filename] = line_count
  return line_count


def main():
  ninja_path = SearchPath('ninja.exe')

  try:
    log_file = '.ninja_log'
    print('Reading .ninja_log file.', file=sys.stderr)
    with open(log_file, 'r') as log:
      entries = ReadTargets(log, False)
      # Create a dictionary of build-times by target
      print('Creating durations database.', file=sys.stderr)
      durations = {}
      for entry in entries:
        for target in entry.targets:
          durations[target] = entry.Duration()
  except IOError:
    print('Log file %r not found, no build summary created.' % log_file, file=sys.stderr)
    sys.exit(0)

  print('Reading dependencies list.', file=sys.stderr)
  command = '%s -t deps' % ninja_path
  deps_lines = os.popen(command).readlines()

  # Dictionary of source file names to line counts.
  line_counts = {}
  # Dictionary of object file names to a list of dependencies (header files
  # included). Note that files pulled in redundantly or pulled in by precompiled
  # header files do not count.
  deps = {}

  active_obj = None
  for line in deps_lines:
    if line.count(':') > 0:
      active_obj = line.split(':')[0]
      active_deps = []
    elif len(line) > 2:
      active_deps += [line.strip()]
    elif active_obj:
      deps[active_obj] = active_deps
      active_obj = None

  print('Reading build commands.', file=sys.stderr)
  command = '%s -t commands chrome' % ninja_path
  command_lines = os.popen(command).readlines()

  print('Generating .csv file.', file=sys.stderr)
  print('name,t_ms,num_dependent_lines,num_lines,num_deps')
  for line in command_lines:
    if line.count('clang-cl.exe') > 0:
      parts = line.split(' ')
      obj_name = parts[-2][3:]
      source_name = parts[-3].replace('/', '\\')
      active_deps = []
      if obj_name in deps:
        active_deps = deps[obj_name]
      line_count = GetLineCount(line_counts, source_name)
      total_dep_line_count = 0
      for dep in active_deps:
        dep_line_count = GetLineCount(line_counts, dep)
        total_dep_line_count += dep_line_count
      print('%s,%d,%d,%d,%d' % (source_name, int(durations[obj_name] * 1000), total_dep_line_count, line_count, len(active_deps)))


if __name__ == '__main__':
    sys.exit(main())
