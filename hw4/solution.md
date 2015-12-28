# Homework 4

## Part One

This is what `sys_sbrk()` looks like after the modification:
```C
int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  proc->sz += n;
  return addr;
}
```

We're going to get a page fault as soon as we invoke any user program on the xv6 prompt. The code will try to reference a memory address which it thinks it has access to and the MMU will generate a hardware interrupt.

## Part Two

I added the following IF statement for lazy page allocation:

```C
// If this is a page fault, allocate new page and map to VA
if (tf->trapno == T_PGFLT) {
    char *mem = kalloc();
    if (!mem)
        panic("we've run out of memory\n");
    memset(mem, 0, PGSIZE);
    mappages(proc->pgdir, (char *) PGROUNDDOWN(rcr2()), PGSIZE, v2p(mem),PTE_W|PTE_U);
    return;
}
```

The homework instructions were detailed and accurate so I don't feel like any additional explanation is necessary.
