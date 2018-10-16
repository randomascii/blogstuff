"""
This script converts two %time% compatible strings passed to it into seconds,
subtracts them, and prints the difference, along with the rest of the arguments.
"""

import re
import sys

def ParseTime(time_string):
  # Time looks like 15:19:30.32 or 15:19:30,32 depending on locale
  # (and there might be other variants as well)
  match = re.match("(.*):(.*):(.*)[\.,](.*)", time_string)
  hours, minutes, seconds, fraction = map(int, match.groups())
  return hours * 3600 + minutes * 60 + seconds + fraction * .01

print "%1.2f seconds elapsed time %s" % (ParseTime(sys.argv[1]) -
                                         ParseTime(sys.argv[2]),
                                         " ".join(sys.argv[3:]))
