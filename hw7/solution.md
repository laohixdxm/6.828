# Homework 7

## Question 1

If you attempt to acquire the same lock in succession, the application would deadlock. Specifically, in the given example, the second `acquire()` would never succeed and hence never return. It would just spin the CPU forever waiting on someone to release the lock.

## Question 2

Tracing the list of `eip` pointers from the panic, I get the following stack trace:

```
proc.c:sched():313
proc.c:yield():329
trap.c:trap():130
trapasm.S:alltraps:23
...
...
```

The kernel panicked because one of the assumptions in `sched()` was violated, namely that only one lock must be held when entering the method. In reality however, two locks were held: `ptable.lock` held by `yield()` (this is the process that interrupted us via a timer interrupt while in `iderw()`) and `idelock` held by `iderw()`. The `cpu->ncli` field is incremented every time a lock is acquired and every time a lock is released is order to match both operations **so that interrupts are enabled only when no locks are held by the kernel.** Since `sched()` assumes exactly one lock is held when it is entered, the value of `cpu->ncli` it expects is exactly one. In reality that value was two because our work in the IDE driver was interrupted while we were holding a lock.

The kernel disables interrupts while holding locks because locks protect invariants about kernel data structures. If an interrupt happened in the middle of an operation modifying an important data structure, like the process table, then the interrupt handler code may see inconsistent state.

## Question 3

The kernel did not panic. I suspect that's due to the relatively shorter time it takes to allocate a file structure as dopposed to reading/writing to the drive. The latter probably taking orders of magnitude longer, thus increasing the chance of hitting a timer interrupt. In fact, if you break in `filealloc()` then the kernel panics (took a couple of tries) because we hit the interrupt timer. So re-enabling interrupts while locks are held is still incorrect, it's just that for the critical section in `filealloc()` we are lucky and get away with it.

## Question 4

Those operations are done prior to releasing the lock because otherwise the lock invariant would be violated. If you released the lock before resetting its state, another user could acquire it in an inconsistent state because the `lk->cpu` and `lk->pcs[]` fields would not correspond to the new acquirer. That would be fine if nothing went wrong and those fields weren't read. The acquirer overwrites them soon after acquiring the lock. However, if there was a panic between the time the new acquirer got the lock and the time it overwrote those fields, the debugging info printed from `lk->cpu` and `lk->pcs[]` would look correct, but actually be incorrect.

In addition, the `xchg` instruction ensures that all operations before it do not get re-ordered after it by the compiler. This makes sure that the lock state is reset before it can get re-acquired again. If those fields were reset after the `xchg` then the compiler could arbitrarily re-order further down in the code's execution therefore extending the period for which the lock would be in an inconsistent state.
