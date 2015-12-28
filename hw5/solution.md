# Homework 5

This is the code I added to `trap.c`:

```C
if (proc && (tf->cs & 3) == 3) {
  proc->elapsedticks += 1;
  if (proc->elapsedticks == proc->alarmticks) {
    proc->elapsedticks = 0;
    tf->esp -= 4;
    *(uint *) tf->esp = tf->eip;
    tf->eip = (uint) proc->alarmhandler;
  }
}
```

The code manipulates the trapframe such that when we return from the current interrupt, the alarm handler code gets executed, followed by a return to the code in user space that was executing when the interrupt happened.

Let's call the `eip` to which we ultimate need to return in `main`, `eip-orig`, and the `eip` of the alarm handler `eip-handler`. The trapframe contains the stack pointer to which we go back to and `eip-orig`. We extend this `esp` by 4 bytes and place `eip-orig` there, thus simulating a function call. We also overwrite `eip-orig` in the trapframe with `eip-handler`, forcing `trapret` to begin executing the alarm handler. The last instruction of the alarm handler assembly code is a `ret`, which pops off a 4 byte word into the `eip`. Recall that we placed `eip-orig` there. The result is that after the alarm handler returns, we continue executing in the original user space code before the interrupt.

This is pretty hacky and there are security concerns associated with forcing the kernel to resume execution at a user supplied `eip` and manipulating the `esp`. One of the checks the kernel can do is to make sure that the `eip-handler` and the new `esp` fall within the size of the process (proc->sz).
