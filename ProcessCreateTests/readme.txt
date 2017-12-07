This test program goes with this blog post:

https://randomascii.wordpress.com/2017/07/09/24-core-cpu-and-i-cant-move-my-mouse/

Be careful when interpreting the results, especially the lock times. Lock
contention gets worse over time so a freshly rebooted machine will *seem* like
it has been fixed. Only an ETW trace (instructions in the blog post) can tell
when the bug is fixed.
