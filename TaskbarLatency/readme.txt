This trace captures right-clicking on two explorer icons on the Windows 10 task bar.
There is a ~600 ms delay and that delay is filed with RuntimeBroker.exe consuming
CPU time calling ReadFile for tiny reads.

See the blog post for details:

https://randomascii.wordpress.com/2019/09/08/taskbar-latency-and-kernel-calls/
