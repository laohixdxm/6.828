# Homework 8

GDB breaks before starting the shell but after we set the new symbol file and a breakpoint at `thread_switch` because the `thread_switch` symbol is located at `0x20c`, which is an address in user space and which gets executed by the newly forked shell process. If you open `sh.asm` you'll see that at `0x20c` you are in the `getcmd()` function.

The `0x161` address at the top of the stack is the `eip` of the function that the thread will execute. That address is the argument to `thread_create()` and points to the `mythread()` function.

See the comments in the `uthread_switch.S` for more details on the assembly.
