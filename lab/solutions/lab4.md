# Lab 4

## Exercise 1

Code is done.

## Exercise 2

The `MPBOOTPHYS` macro is needed because `mpentry.S` is linked at high addresses but gets loaded by `boot_aps()` at the low address `MPENTRY_ADDR`. The bootloader doesn't need a macro like this because it is linked and loaded at the same low address (`0x00007c00`). You can verify this by running `objdump -h obj/boot/boot.out` and comparing the VMA and LMA columns. If `mpentry.S` didn't make use of a macro like `MPBOOTPHYS` then the symbols it references wouldn't work at runtime because they have been addressed (linked) at high addresses and the macro translates them to addresses relative to where the code actually gets loaded, namely `MPENTRY_ADDR`.

## Exercise 3

Code is done.

## Exercise 4

Code is done.

## Exercise 5

We still need separate stacks for each CPU because during a trap/interrupt, the trapframe is pushed onto the stack **without holding the kernel lock**. This means that, say CPU 1 enters the kernel on a system call and while it is in the kernel, CPU 2 attempts to enter the kernel on a timer interrupt. CPU 2 can't enter the kernel, it will be spinning at the lock we just added in `trap()`. However, it will have pushed its trap frame **on top of the trap frame already pushed by CPU 1**. This of course means that when CPU 1 returns to user mode, it will pop off CPU 2's frame and return in that environment instead of its own.

## Exercise 6

The pointer to the `struct Env` about to be run, works after we switch page tables because all environment page directories share certain mappings. That pointer points somewhere in `UENVS`, which is shared. The `envs` array is allocated and mapped to `UENVS` in `mem_init()` and those mappings are then **copied to all new page directories** in `env_setup_vm()`.

User environments have their registers saved by the kernel because they need to be able to resume seamlessly (this includes getting back to the right stack `esp` and at the right instruction `eip`) and their interruption is not always predictable. Processes keep temporary data and variables in registers and the process assumes that when it returns, all those values will still be there. It's a like the caller/callee save calling convention except here it is for interrupting the whole process.

The registers are saved on the user environment's stack as part of the trapframe constructed by the `int` instruction and the code in `alltraps`. To restore the state of a new process, JOS uses the `env_pop_tf()` function, which switches first to the new process' stack and the pops all the registers in place.

## Exercise 7

Code is done and `dumbfork` works correctly.

## Exercise 8

Code is done.

## Exercise 9

Code is done. If the user environment runs out of space on the exception stack, a page fault will be generated, which will end up in `page_faul_handler()` where the code I just wrote will detect that we've overflown the stack space and will kill the process.

## Exercise 10 & 11

Code is done. `user/faultalloc` and `user/faultallocbad` behave differently because the latter checks memory permissions before dereferencing said memory. Since the memory address referenced has not yet been mapped, the assertion fails and the process is destroyed. On the other hand `user/faultalloc` dereferences the memory location directly through `cprintf()`, which allows our handler to process the fault.

## Exercise 12

Code is done. This exercise took me a while because I had to go back to code I wrote in previous labs and fix some things. For example, I was setting the wrong permissions on PDE, causing page faults when trying to re-map those entries in `duppage()`. In addition, I was creating the PDE in `boot_map_region()` instead of the place where those entries are initially mapped, namely in `pgdir_walk()`.

## Exercises 13, 14 & 15

Code is done. These took a while as well because again I had to go back in previous code I had written and fix mistakes :). For example, the timer interrupt was being masked even after I explicitly set the interrupt flag in `env_alloc()`. It turns out I was overwriting the EFLAGS value in `load_icode()`. While reading the ELF spec I noticed that as part of the header there's an EFLAGS value, with which the process expects to start executing. That value was always 0 and therefore cleared the interrupt flag I was setting before.

I also had an incorrect scheduler. Instead of starting from the next process after the current one, the loop always started from the beginning.

## Challenge

I chose to extend the scheduler. I introduced three levels of scheduling priority: `ENV_HI_PRIORITY`, `ENV_MED_PRIORITY` and `ENV_LOW_PRIORITY`. All environments are created with a medium priority unless explicitly set. I chose to set the priority via the `env_create()` function. An alternative would be to use system calls so that processes can change their own priority. This is very similar to Linux's `nice` utility, which allows users to set process priority.

The new scheduler will first look for an `ENV_HI_PRIORITY` environment that is runnable, **other than the environment currently executing**. If it doesn't find one, it will drop priority to `ENV_MED_PRIORITY` and so on. This prolongs the runtime of scheduling an environment from O(n) to 3*O(n), the factor of three coming from the three priority levels. This is too slow for a real, production kernel where more efficient algorithms and data structures would be required.

A possible improvement would be to use [multi level feedback queues](http://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-mlfq.pdf). The goal of MLFQ is to adaptively adjust the priorities of processes so as to favor (give higher priority to) processes that are short lived (preserving the responsiveness of the system) and those that are I/O bound. The space and runtime complexity of this scheduling algorithm would be only slight better at O(n). Another improvement would be what the Linux kernel is currently running, namely the [Completely Fair Scheduler](https://en.wikipedia.org/wiki/Completely_Fair_Scheduler), which can pick a next process to run in constant time but takes O(logN) to re-insert the process for future scheduling.

In order to test my changes, I spawned four instances of the `yield.c` user program on a single CPU. I gave two instances a high priority, I gave one a medium priority and one a low priority. The results of the run are included below:

```
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
[00000000] new env 00001003
Hello, I am environment 00001001 with priority 0.
Hello, I am environment 00001000 with priority 0.
Back in environment 00001001, iteration 0.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 1.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 2.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 3.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Hello, I am environment 00001002 with priority 1.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001002, iteration 0.
Hello, I am environment 00001003 with priority 2.
Back in environment 00001002, iteration 1.
Back in environment 00001003, iteration 0.
Back in environment 00001002, iteration 2.
Back in environment 00001003, iteration 1.
Back in environment 00001002, iteration 3.
Back in environment 00001003, iteration 2.
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
Back in environment 00001003, iteration 3.
Back in environment 00001003, iteration 4.
All done in environment 00001003.
[00001003] exiting gracefully
[00001003] free env 00001003
No runnable environments in the system!
```

First thing to note is that we're running on 1 CPU. If we ran this experiment on 4 CPUs, then each CPU would run a process, making priorities irrelevant. Ideally, in order to evaluate the effectiveness of priority scheduling you need to run more processes than there are CPUs and it is easiest to judge correctness when you only have 1 CPU. Reminder: a priority of 0 means high and a priority of 2 means low.

We correctly schedule between `00001000` and `00001001`, both of which are high priority, until both have terminated. We then alternate between `00001002` and `00001003`. However, we're scheduling between medium and low priorities here; why don't we run out the medium priority process before jumping on the low priority one? Remember that when we jump into the scheduler from a medium priority process, we look for **another process at the next highest priority** (otherwise a high priority process could just spin the CPU and never relinquish control). In our case, there's nothing other than `00001002` at medium priority, so we have to downgrade to low priority and run `00001003`.
