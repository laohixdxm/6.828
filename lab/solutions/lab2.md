# Lab 2

## Exercise 1

Code is done.

## Exercise 2

Read the relevant sections of the manual.

## Exercise 3

To check that I can read physical addresses using the QEMU monitor commands,
I overwrote the memory of a variable and then examined the physical memory
(obtained by subtracting KERNBASE from the virtual address). Below is a
transcript of what I did:

```
# Both virtual and physical point to 0. Virtual address is 0xf0115fac and
# corresponding physical address is 0x00115fac.
(gdb) p i
$1 = 0
(gdb) p &i
$2 = (size_t *) 0xf0115fac
(qemu) xp 0x00115fac
0000000000115fac: 0x00000000

# Write to variable i and check physical memory
(gdb) set {size_t}0xf0115fac = 0xba5eba11
(gdb) p/x i
$3 = 0xba5eba11
(qemu) xp 0x00115fac
0000000000115fac: 0xba5eba11
```

Answer to question: the type of x will have to be `uintptr_t`. The reason for
that is that the code snippet dereferences the pointer and writes to it. If
`return_a_pointer()` returned a physical address, then when we attempt to write
to it, that address will be interpreted by the MMU as virtual and we'll end up
writing somewhere else. So, assuming the sample code is correct, x must be a
virtual address pointer.

## Exercise 4

Code is done and the check*() functions report success.

## Exercise 5

Code is done and the check*() functions report success.

### Question 2

Three major sections of the linear address space have so far been mapped 1)
the space above KERNBASE, 2) the kernal stack and 3) the pages data structure.
One can easily determine what space is mapped by looking at the calls to
`boot_map_region()` in `mem_init()`. Note that a page table addresses 4MB of
memory and there's a one to one correspondence from page directory entries
to page tables. Thus one page directory entry addresses 4MB of memory.
A nice corollary that follows from this is that the total addressable memory
is 1024 (total num directory entries) * 4MB = 4GB.

One page table addresses = 4 MB = 4194304 bytes = 0x400000 bytes


| PDE | Base Virtual Address | Points to (logically) |
|-----|----------------------|-----------------------|
|1023| 0xFFBFFFFF | Last addressable page table, for kernel use. |
| ... | ... | ... |
| 959 | KERNBASE (0xF0000000) | First page table of kernel, located right at the bottom of physical memory |
| 958 | KERNBASE - PTSIZE (EFC00000) | This is memory for the kernel stack. It's mapped to physical memory equal to bootstack (see bootloader). Note that not all the page table memory is actually mapped, we only map memory equal to KSTACKSIZE (8 pages) and leave the rest unmapped so as to guard against the kernel stack growing too large. |
| ... | ... | ... |
| 955 | UPAGES (0xEF000000) | This is where we mapped the `pages` data structure |
| ... | ... | ... |
| 0 | 0 | Start of virtual memory |

### Question 3

Isolation in the virtual address space is achieved via the permission bits
of both page directory and page table entries. Those bits are checked by the
hardware once paging is enabled. The permission bits are located in the bottom
12 bits of every page table/directory entry.

To be more specific, pages are assigned either one of two privilege levels:
supervisor or user. The current level is related to the CPL (current privilege
level). A CPL of 0, 1 or 2 is equivalent to supervisor and a CPL of 3 is
equivalent to user level. When executing in supervisor mode, all pages are
accessible but when executing in user mode only other user mode pages are
accessible.

### Question 4

This OS can support up to 4GB of memory (see the last part of my answer to
question 1 for an explanation). 4GB is the maximum number of bytes
addressable using 32 bits. 2^32 = 4294967296 bytes = 4 GB. The maximum memory
that can be used by the kernel is 256 MB, because the kernel is mapped from
0xF000000 virtual to 0 physical, meaning that it only has
0xFFFFFFFF - 0xF0000000 = 256 MB at its disposal.

### Question 5

So far the overhead of virtual has been (1) storing the page directory and page
tables (2) storing the pages array (3) having a chunk of memory under the
kernel stack not mapped so as to trigger a page fault in case the stack goes
over.

### Question 6

The transition to a high EIP happens with the jump to the `relocated` tag.
We're able to run at a low EIP after enabling paging because the page directory
setup in `entrypgdir.c` maps virtual addresses 0 - 4MB to physical 0 - 4MB. The
transition is necessary because the rest of the kernel is linked at high
addresses.

## Challenge

I chose to complete the challenge exercise that asked you to add debugging
functions to the kernel monitor. I added two: `showmappings` and `setperms`.
The first method will print the mapping of virtual addresses to their pages,
those pages' physical address and the permissions (both in human readable and
in binary). If the virtual address is not mapped, the function prints out
a message.

`setperms` will set the permissions of the page table and page directory
entry corresponding to the given virtual address. If the virtual adddress is
not mapped, then the function prints an error message. The functions takes
two arguments, the first being the hex virtual address and the second being the
permissions in hex.
