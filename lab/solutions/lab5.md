# Lab 5

## Exercise 1

Nothing else needs to be done in order to ensure this privilege gets preserved as we switch from one environment to another. There's no danger that this flag somehow 'leaks' to other processes. When switching from the file system environment to another environment, the two trapframes are completely separate. If the file system environment forks, then the child would inherit the `EFLAGS` register value and hence the I/O privilege. That is acceptable because presumably the kernel trusts the file system environment not to fork off a malicious child. A random user environment cannot set this flag by itself because it is running at CPL 3 and the CPU only lets processes running at CPL 0 edit the `EFLAGS` register.

## Exercises 2 & 3 & 4

Exercises were pretty straightforward, code is done.
