This test program was inspired by a discussion on twitter about whether !analyze
was useful. It had failed to identify a use-after-unload crash, which apparently
it is supposed to support so I wrote some tests.

The TL;DR is that !analyze can identify this pattern only if the crash happens
when the instruction pointer is in the unloaded DLL. If a data load or store to
the DLL happens then !analyze gives up.

https://twitter.com/ARichardMSFT/status/1153663504856965122

To use this program run it under windbg, specifying one of its three options.
After it crashes run !analyze and see whether it identifies module unload of
crash.dll as the cause.
