# Copyright 2021 Bruce Dawson
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
This tool generates a configurable number of images on the users desktop in
order to trigger an explorer.exe performance bug first officially reported here:
https://github.com/microsoft/WinDev/issues/80
"""

import os
# pip3 install psutil may be needed to make this available.
import shutil
import sys

try:
  import psutil
  psutil_loaded = True
except:
  psutil_loaded = False
  print('psutil not found. Results will be less consistent.')

# Check for Python3 - this is a syntax error under Python 2.
USE_PYTHON_3 = f'This script will only run under python3.'

def main():
  if len(sys.argv) < 2:
    print('  Usage: %s file_count' % __file__)
    print('Copies the requested number of pictures to the desktop. This is')
    print('useful for reproducing poor scaling of icon layout')
    return 0

  file_count = int(sys.argv[1])
  if file_count > 1000:
    print('%d pictures is a lot. A thousand is more than enough to reproduce'
          % file_count)
    print('the bug. Edit the script to remove the limitation if you\'re really')
    print('sure.')
    return 0

  script_path = os.path.dirname(os.path.realpath(__file__))
  desktop_path = os.path.join(os.environ['userprofile'], 'Desktop')
  src = os.path.join(script_path, 'SunsetWhales.jpg')
  dst = os.path.join(desktop_path, 'TestFiles%04d.jpg')
  print('Copying %d files from %s to %s.' % (file_count, src, dst))

  if psutil_loaded:
    for proc in psutil.process_iter():
        if proc.name() == 'explorer.exe':
            explorer_pid = proc.pid
    explorer = p = psutil.Process(explorer_pid)
    # Suspend the explorer process so that it processes all of the new images at
    # once.
    explorer.suspend()
  for i in range(file_count):
    shutil.copy(src, dst % i)
  if psutil_loaded:
    explorer.resume()
  print()
  print('Don\'t forget to clean up the files when you\'re done with them with')
  print('"del %s" or similar.' % dst.replace('%04d', '*'))

if __name__ == '__main__':
  sys.exit(main())
