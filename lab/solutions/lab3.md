# Lab 3

## Exercise 1

Code is done. Nothing too fancy, it's almost a copy of what was done for the `pages` array.

## Exercise 2

Code is done. I was lucky to find the `memmove` function in `string.c`. I was about to start copying the binary image manually using `rep movs` inline assembly, until I grepped the codebase to find examples of `rep movs` usage and ran into `memmove`. It would have been nice to get a hint about the existence of `memmove`.

A short note about setting up and jumping into the new environment trapframe. `env_alloc` sets the first part of the frame, including the `ss` and `esp` values. Note that `env_alloc` sets `esp` to `USTACKTOP` and in `load_icode`, `USTACKTOP` is mapped to a newly allocated page so nothing more needs to be done for getting the right user stack. `load_icode` also sets the `EFLAGS` values in the trapframe by reading them from the binary (recall that the ELF header contains those flags in the `e_flags` field). It also sets the trapframe's `eip` to the entry point specified in the ELF header. Finally, the `env_pop_tf` function takes the created trapframe and pops it all into the appropriate registers so that the user process can start executing. `env_pop_tf` utilizes a little hack to make the switch happen by overwriting the `esp` to point to the trapframe before starting to pop all the values.
