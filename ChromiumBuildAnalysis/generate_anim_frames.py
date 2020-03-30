"""
Generate a set of build-time .csv files by doing a lerp (with ease-in/ease-out)
between two other build-time .csv files.

The input files and the number of frames are hard-coded because I don't plan to
use this script again.

The order of build steps must match between the two .csv files and this is
checked using asserts.
"""

from __future__ import print_function

import argparse
import math
import sys


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('start_file', nargs=1)
  parser.add_argument('end_file', nargs=1)
  parser.add_argument('--num_frames', type=int, default=45)

  args = parser.parse_args()

  with open(args.start_file[0], 'r') as f:
    default = f.readlines()
  with open(args.end_file[0], 'r') as f:
    pch_fix = f.readlines()

  assert(len(default) == len(pch_fix))

  for i in range(args.num_frames):
    # Use math.cos to create t values that do smooth acceleration and deceleration
    # to avoid jarring jerks in the animation.
    theta = i * math.pi / (args.num_frames - 1)
    t = (math.cos(theta) + 1) / 2

    with open('anim-frame%d.csv' % i, 'w+') as f:
      f.write('name,t_ms,num_dependent_lines,num_lines,num_deps\n')

      t2 = 1.0 - t
      for line_num in range(1, len(default)):
        def_parts = default[line_num].split(',')
        fix_parts = pch_fix[line_num].split(',')
        # There's no guarantee things will happen in the same order, but they do?
        # That's a good thing because many file names generate multiple .obj files and
        # therefore show up multiple times so matching them up would otherwise be
        # tricky.
        assert(def_parts[0] == fix_parts[0])
        output = [def_parts[0]]
        for x in range(1, 5):
          output.append(int(int(def_parts[x]) * t + int(fix_parts[x]) * t2))
        f.write('%s,%d,%d,%d,%d\n' % tuple(output))

if __name__ == '__main__':
    sys.exit(main())
