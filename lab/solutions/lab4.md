# Lab 4

## Exercise 1

Code is done.

## Exercise 2

The `MPBOOTPHYS` macro is needed because `mpentry.S` is linked at high addresses but gets loaded by `boot_aps()` at the low address `MPENTRY_ADDR`. The bootloader doesn't need a macro like this because it is linked and loaded at the same low address (`0x00007c00`). You can verify this by running `objdump -h obj/boot/boot.out` and comparing the VMA and LMA columns. If `mpentry.S` didn't make use of a macro like `MPBOOTPHYS` then the symbols it references wouldn't work at runtime because they have been addressed (linked) at high addresses and the macro translates them to addresses relative to where the code actually gets loaded, namely `MPENTRY_ADDR`.
