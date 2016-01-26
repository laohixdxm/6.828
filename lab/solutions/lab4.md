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
