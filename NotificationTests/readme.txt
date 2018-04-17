Using the FileSystemWatcher class with too large a buffer size
(InternalBufferSize) will make all file operations in monitored
directories slower, potentially hugely slower.

This commit includes a Python script that creates and deletes files
in order to provide a test bed and some ETW traces showing the
results.

A detailed explanation can be found on my blog at:
https://randomascii.wordpress.com/2018/04/17/making-windows-slower-part-1-file-access/

The Python script (FileStress.py) creates and then deletes 1,000 files
in one directory, and then another, with a half-second gap in-between.
To recreate this problem you need to have VsChromium 0.9.26 installed
and monitoring the first of these directories.

Or, just look at these three traces:
- 2018-04-11_18-59-57 Notification Testing 0.9.26.etl
This trace captures running the Python script once with version 0.9.26
of VsChromium installed and monitoring one directory. Context switch
callstacks are off.

- 2018-04-11_20-18-03 Notification Testing 0.9.26 - ALL_FAULTS.etl
This trace captures the same scenario but with the ALL_FAULTS provider
enabled.

2018-04-11_19-04-51 Notification Testing 0.9.30.etl
This trace captures the same scenario but with version 0.9.30 of
VsChromium installed.
