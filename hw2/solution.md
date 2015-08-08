# Homework 2

I broke up the `x/24x $esp` into one byte per line so it is easier to comment.
Also, note that we don't need to trace through any intermediate calls in
`bootmain`because when they return the will leave the stack in the same state
as before (`call` instruction pushes the return EIP and the `ret` instruction
will pop that address back into the EIP).

```
Top of the stack, contains return EIP of when the kernel returns (should never
happen).  
0x7bcc: 0x00007db7
The next 7 bytes were allocated at the beginning of bootmain to be used as
scratch space for local variables and for passing arguments to functions called
0x7bd0: 0x00000000
0x7bd4: 0x00000000
0x7db8: 0x00000000
0x7bdc: 0x00000000
0x7de0: 0x00000000
0x7de4: 0x00000000
0x7de8: 0x00000000
0x7bec: 0x00000000 <- Callee saved EBX by bootmain
0x7bf0: 0x00000000 <- Callee saved ESI by bootmain
0x7bf4: 0x00000000 <- Callee saved EDI by bootmain
0x7bf8: 0x00000000 <- EBP of bootasm frame, bootmain ESP initialized here
0x7bfc: 0x00007c4d <- EIP of instruction after call to bootmain

Any addresses above this are not defined and not part of the stack.
```
