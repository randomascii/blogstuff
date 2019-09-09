This trace captures right-clicking on two explorer icons on the Windows 10 task bar.
There is a ~600 ms delay and that delay is filed with RuntimeBroker.exe consuming
CPU time calling ReadFile for tiny reads.
