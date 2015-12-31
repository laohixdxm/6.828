# Lab 3

## Exercise 1

Code is done. Nothing too fancy, it's almost a copy of what was done for the `pages` array.

## Exercise 2

Code is done. I was lucky to find the `memmove` function in `string.c`. I was about to start copying the binary image manually using `rep movs` inline assembly, until I grepped the codebase to find examples of `rep movs` usage and ran into `memmove`. It would have been nice to get a hint about the existence of `memmove`.

A short note about setting up and jumping into the new environment trapframe. `env_alloc` sets the first part of the frame, including the `ss` and `esp` values. Note that `env_alloc` sets `esp` to `USTACKTOP` and in `load_icode`, `USTACKTOP` is mapped to a newly allocated page so nothing more needs to be done for getting the right user stack. `load_icode` also sets the `EFLAGS` values in the trapframe by reading them from the binary (recall that the ELF header contains those flags in the `e_flags` field). It also sets the trapframe's `eip` to the entry point specified in the ELF header. Finally, the `env_pop_tf` function takes the created trapframe and pops it all into the appropriate registers so that the user process can start executing. `env_pop_tf` utilizes a little hack to make the switch happen by overwriting the `esp` to point to the trapframe before starting to pop all the values.

## Exercise 3

Read the relevant sections of the Intel manual.

## Exercise 4

1. The purpose of having a separate handler function for each exception/interrupt is to maintain privilege isolation between user space and the kernel. The handler's function is twofold: (a) pass arguments to the code that is going to handle the exception/interrupt and (b) determine which piece of code will handle the exception/interrupt. By hardcoding both of these pieces of information in the handler functions, the kernel designer prevents user processes from having any influence over the control transfer/privilege escalation.

  If all interrupts were delivered to the same handler, the kernel would have no way to tell which exception/interrupt occurred. That information is stored in the trapframe and is placed there by the handler code which we dispatch to via the IDT. There's no mechanism to pass information to the handler from the IDT during the dispatch. The only way to establish a correspondence is to have a one to one mapping from the IDT descriptor to the handler to the interrupt.

2. I didn't have to do anything extra to make `user/softint` work. `softint` produces interrupt 13 because it doesn't have permissions to invoke a page fault (`int $14`). When setting up the IDT in `trap_init`, interrupt 14 is mapped so that only code at CPL 0 can invoke it. `softint` is running in user space and therefore has a CPL of 3 and is not allowed to invoke `int $14`. The processor catches this privilege mismatch and raises a general protection fault (`int $13`). Note that this is not a nested interrupt, the page fault requested from `softint` is never processed. I verified this by setting a breakpoint at both the `int $14` and `int $13` handlers and never hit the handler for `int $14`.

  As written at this stage of the lab, the page fault handler of the kernel will shut down any user process that invokes it. Allowing user processes to manipulate virtual memory by asking for pages to be mapped is a major violation of isolation and should not be allowed.

## Exercise 5

To support page faults, I added a case statement in `trap_dispatch` that calls `page_fault_handler`. More importantly however, the user test code uncovered a bug in my page mappings. I was mapping all pages above `KERNBASE` with user privileges.
