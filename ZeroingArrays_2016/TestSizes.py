"""
Test compilation of char array[BUF_SIZE] = {} versus char array[BUF_SIZE] = {0}
"""

import os
import subprocess
import sys

terse = False
if len(sys.argv) > 1:
  if sys.argv[1].lower() == "--terse":
    terse = True
  else:
    print "--terse is the only option to this program"

test_code = r'''// Test of code generation
#include <stdio.h>

void ZeroArray1()
{
	char buffer[BUF_SIZE] = { 0 };
	printf("Don't optimize away my empty buffer.%s\n", buffer);
}

void ZeroArray2()
{
	char buffer[BUF_SIZE] = {};
	printf("Don't optimize away my empty buffer.%s\n", buffer);
}
'''

open("TestCode.cpp", "w").write(test_code)

os.environ["INCLUDE"] = r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\INCLUDE;C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\ATLMFC\INCLUDE;C:\Program Files (x86)\Windows Kits\10\include\10.0.10586.0\ucrt;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\include\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.10586.0\shared;C:\Program Files (x86)\Windows Kits\10\include\10.0.10586.0\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.10586.0\winrt;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\INCLUDE;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\ATLMFC\INCLUDE;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\include"

startpath = os.environ["PATH"]

for bitness in [32, 64]:
  for options in ["/O1 /Oy-", "/O2 /Oy-", "/O2"]:
    if bitness == 64:
      options = options.replace(" /Oy-", "")
    print "%d-bit with %s:" % (bitness, options),
    if not terse:
      print
    if bitness == 32:
      os.environ["PATH"] = r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\BIN;" + startpath
    else:
      os.environ["PATH"] = r"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\BIN\AMD64;" + startpath
    total = 0
    count = 0
    last_diff = None
    total_sizes = [(0,0)]
    for buf_size in range(1,65):
      # Using /O2 instead of /O1 changes the results
      # On 32-bit /O2 buids /Oy- changes the results
      # /Oi seems to make no difference
      command = "cl /nologo /c TestCode.cpp %s /FAcs /DBUF_SIZE=%d" % (options, buf_size)
      subprocess.check_output(command)
      lastline = ""
      size1 = None
      size2 = None
      SSE_used = False
      SSE_used_for1 = None
      SSE_used_for2 = None
      for line in open("TestCode.cod").readlines():
        if line.count(" PROC") > 0:
          SSE_used = False
        if line.lower().count("xmm") > 0:
          SSE_used = True
        if lastline.count("ret") == 1:
          if line.startswith("?ZeroArray1@@YAXXZ"):
            size1 = int(lastline.strip().split("\t")[0], 16) + 1
            SSE_used_for1 = SSE_used
            SSE_used = False
          if line.startswith("?ZeroArray2@@YAXXZ"):
            size2 = int(lastline.strip().split("\t")[0], 16) + 1
            SSE_used_for2 = SSE_used
            SSE_used = False
        lastline = line

      last_total = total_sizes[-1]
      total_sizes.append((last_total[0] + size1, last_total[1] + size2))
      if size1 != size2:
        last_diff = buf_size
      if not terse:
        print "%2d: %2d -> %2d: %+3d bytes" % (buf_size, size1, size2, size2 - size1),
        print "%4.1f%%" % ((size1 - size2) * 100.0 / size1),
        if SSE_used_for1:
          print " SSE used for = {0};",
        if SSE_used_for2:
          print " SSE used for = {};",
        print
      total += size2 - size1
      count += 1

    print "Average saving from 1 to %d is %1.3f bytes," % (last_diff, total * -1.0 / last_diff),
    last_diff_totals = total_sizes[last_diff]
    print "%1.2f%%" % ((last_diff_totals[0] - last_diff_totals[1]) * 100.0 / last_diff_totals[0])
    if not terse:
      print
  if not terse:
    print
