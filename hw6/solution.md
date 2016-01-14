# Homework 6

Why do some keys start missing when 2 or more threads are run simultaneously? Suppose thread A and B run concurrently. If both threads insert keys that fall into the same bucket then a race may occur. The events below outline the operations that will lead to such an outcome.

t-A: calls `insert()` on bucket 1 (key = 6, 6 % NBUCKETS = 1)

t-B: calls `insert()` on bucket 1 (key = 21, 21 % NBUCKETS = 1)

...

...

t-A: executes `e->next = n`

t-B: executes `e->next = n`

t-A: executes `*p = e`

t-B: executes `*p = e`

The result of this is that the last thread to execute `*p = e`, effectively overwrites the action of the previous thread and sets the new head of the list. Following the execution order above, key 6 supposedly inserted by thread A will be missing because thread B overwrote it.

The following runtimes are derived from runs without any mutual exclusion.

```
anton@laptop:~/.../hw$ ./a.out 1
0: put time = 0.005017
0: lookup time = 8.665682
0: 0 keys missing
completion time = 8.671127

anton@laptop:~/.../hw$ ./a.out 2
0: put time = 0.002287
1: put time = 0.002368
1: lookup time = 8.375960
1: 0 keys missing
0: lookup time = 8.377120
0: 0 keys missing
completion time = 8.387772
```

In order to prevent the race condition outlined earlier, we can place locks around each `put()` and each `get()` operation. This is definitely suboptimal because it eliminates all parallelism. Later on in this assignment we'll improve our solution. After running the application with our newly introduced locks, we get the following runtimes.

```
anton@laptop:~/.../hw$ ./a.out 1
0: put time = 0.008384
0: lookup time = 7.908485
0: 0 keys missing
completion time = 7.917301

anton@laptop:~/.../hw$ ./a.out 2
1: put time = 0.015524
0: put time = 0.016734
1: lookup time = 19.783152
1: 0 keys missing
0: lookup time = 19.862858
0: 0 keys missing
completion time = 19.879917
```

You'll notice that the 2 thread run takes much longer than the 1 thread run. In fact it takes longer than if each thread ran sequentially (that would take around 8.38 * 2 ~= 17 seconds). Where did we spend the extra 19.8 - 17 = 2.8 seconds? Probably in synchronization overhead for unlocking and locking. You'll also notice that no keys are missing because only one thread can ever execute `insert()`.

Locking `put()` and `get()` is too conservative. We can preserve correctness and re-introduce parallelism. The first improvement is to remove locking in `get()`. The `get()` operation is a read operation and will not interfere with any write operation. Note that both threads must finish their `put()` phases before continuing to the `get()` phase so we're guaranteed that there won't be any read-write or write-write conflict. Below are the runtimes after removing locks from `get()`:

```
anton@laptop:~/.../hw$ ./a.out 1
0: put time = 0.008592
0: lookup time = 7.725541
0: 0 keys missing
completion time = 7.734578

anton@laptop:~/.../hw$ ./a.out 2
0: put time = 0.004287
1: put time = 0.003956
0: lookup time = 7.603547
0: 0 keys missing
1: lookup time = 7.608980
1: 0 keys missing
completion time = 7.618153
```

We've preserved correctness (0 keys missing) and re-introduced parallelism so that the 2 thread run is faster than the 1 thread run. We can further improve parallelism by not locking the entire `table` array for every `put()` but instead only locking the bucket we're inserting into. This allows other threads to concurrently insert into the same `table` but in a different bucket. I believe this technique is called lock splitting and is used in the implementation of Java's `ConcurrentHashMap`. Below are the run times with locks for each bucket:

```
anton@laptop:~/.../hw$ ./a.out 1
0: put time = 0.008746
0: lookup time = 8.085426
0: 0 keys missing
completion time = 8.094725

anton@laptop:~/.../hw$ ./a.out 2
1: put time = 0.004895
0: put time = 0.009363
0: lookup time = 7.827121
0: 0 keys missing
1: lookup time = 7.825160
1: 0 keys missing
completion time = 7.840231
```

Correctness has been preserved and the 2 thread run is even faster than the 1 thread run. Take a look at `ph.c` for the code of the final solution.
