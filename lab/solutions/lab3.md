# Lab 3

## Exercise 1

Code is done. Nothing too fancy, it's almost a copy of what was done for the `pages` array.

## Exercise 2

Code is done. I was lucky to find the `memmove` function in `string.c`. I was about to start copying the binary image manually using `rep movs` inline assembly, until I grepped the codebase to find examples of `rep movs` usage and ran into `memmove`. It would have been nice to get a hint about the existence of `memmove`.

A short note about setting up and jumping into the new environment trapframe. `env_alloc` sets the first part of the frame, including the `ss` and `esp` values. Note that `env_alloc` sets `esp` to `USTACKTOP` and in `load_icode`, `USTACKTOP` is mapped to a newly allocated page so nothing more needs to be done for getting the right user stack. `load_icode` also sets the `EFLAGS` values in the trapframe by reading them from the binary (recall that the ELF header contains those flags in the `e_flags` field). It also sets the trapframe's `eip` to the entry point specified in the ELF header. Finally, the `env_pop_tf` function takes the created trapframe and pops it all into the appropriate registers so that the user process can start executing. `env_pop_tf` utilizes a little hack to make the switch happen by overwriting the `esp` to point to the trapframe before starting to pop all the values.

## Exercise 3

Read the relevant sections of the Intel manual.

## Exercise 4

1. The purpose of having a separate handler function for each exception/interrupt is to maintain privilege isolation between user space and the kernel. The handler's function is twofold: (a) pass arguments to the code that is going to handle the exception/interrupt and (b) determine which piece of code will handle the exception/interrupt. By hardcoding both of these pieces of information in the handler functions, the kernel designer prevents user processes from having any influence over the control transfer/privilege escalation. A safe and secure way to establish a correspondence is to have a one to one mapping from the IDT descriptor to the handler, to the interrupt.

  If all interrupts were delivered to the same handler, there would be no way to assign different permissions to different handlers. For example, you want user code to be able to invoke `T_BRKPT` and `T_SYSCALL` so for those handlers you set DPL in the IDT to 3 but you don't want user code to invoke `T_PGFLT` so for that handler you set DPL to 0.

2. I didn't have to do anything extra to make `user/softint` work. `softint` produces interrupt 13 because it doesn't have permissions to invoke a page fault (`int $14`). When setting up the IDT in `trap_init`, interrupt 14 is mapped so that only code at CPL 0 can invoke it. `softint` is running in user space and therefore has a CPL of 3 and is not allowed to invoke `int $14`. The processor catches this privilege mismatch and raises a general protection fault (`int $13`). Note that this is not a nested interrupt, the page fault requested from `softint` is never processed. I verified this by setting a breakpoint at both the `int $14` and `int $13` handlers and never hit the handler for `int $14`.

  As written at this stage of the lab, the page fault handler of the kernel will shut down any user process that invokes it. Allowing user processes to manipulate virtual memory by asking for pages to be mapped is a major violation of isolation and should not be allowed.

## Exercise 5

To support page faults, I added a case statement in `trap_dispatch` that calls `page_fault_handler`. More importantly however, the user test code uncovered a bug in my page mappings. I was mapping all pages above `KERNBASE` with user privileges.

## Exercise 6

To support user breakpoints I added another case statement to `trap_dispatch` that prints the trapframe and then drops into the monitor. I also chose to tackle the single step debugging challenge exercise, whose solution is detailed at the end under the [Challenge](##challenge) heading.

1. I originally setup the IDT with the correct permissions (DPL = 3) for the breakpoint interrupt. I was aware of this because while reading the IDT setup code in xv6 I noticed that the syscall interrupt gate was given different permissions (DPL = 3) because it was expected to be invoked by user level code. If I had setup the breakpoint interrupt with DPL = 0, like the other interrupts, then the `breakpoint` user code would have generated a general protection fault.

2. The point of these mechanisms is to restrict the influence user level code can have on the kernel. User code can ask for services (syscalls), set breakpoints but cannot manipulate virtual memory (`int $14` in `softint`).

## Exercise 7

Code is complete.

## Exercise 8

Code is complete.

## Exercise 9 & 10

Code is complete and `make grade` passes all tests.

The reason why `backtrace` generates a page fault after it reaches `libmain.c` is because that is the top most stack frame of the user process. If you look at the faulting address, `0xeebfe000`, you'll notice that that is `USTACKTOP`. Therefore, when `backtrace` attempted to dereference the stack frame above `libmain.c`, namely above `USTACKTOP`, it ran into an empty page and caused a page fault.

## Challenge

To enable the single stepping of instructions, I set the `FL_TF` (trap) flag in the `EFLAGS` register. I also added a case statement in `trap_dispatch` to catch `T_DEBUG` interrupts, which are the interrupts generated by the CPU when `FL_TF` is on.

I added two functions to the JOS monitor to aid in the debugging: `singlestep` and `continue`. The `singlestep` function sets the `FL_TF` flag in the `tf_eflags` portion of the trapframe, which is what gets popped into the `EFLAGS` register once execution of the user process resumes. After a single instruction in the user process, the CPU interrupts with a `T_DEBUG`, which gets caught in `trap_dispatch` and the user is dropped into the monitor once again. At this point, the user can `singlestep` again or execute a `continue` command, which turns off the `FL_TF` flag in `tf_eflags` and resumes execution of the user process until completion.

I tested my solution by looking at the `eip` in the trapframe at every break and making sure that it matched the next instruction in `obj/user/breakpoint.asm`.
