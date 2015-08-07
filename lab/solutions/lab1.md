# Lab1

## Exercise 1

To learn assembly I read the
[PC Assembly Book](http://www.drpaulcarter.com/pcasm/) linked in the resources
section. I also watched the lectures for the Intro to x86 class over at
opensecuritytraining.info. As a final test I reverse engineered the CMU binary
bomb. You can find my solution
[here](https://github.com/petroav/CMU-assembly-challenge).

## Exercise 2

Here are the disassembled BIOS instructions with my comments
inlined:
```
# This is a long jump because we're crossing segments
[f000:fff0]    0xffff0: ljmp   $0xf000,$0xe05b

# I can't tell what's at %cs:0x65b4 and why that's special
[f000:e05b]    0xfe05b: cmpl   $0x0,%cs:0x65b4
[f000:e062]    0xfe062: jne    0xfd3aa

# This has the effect of zeroing out %ax
[f000:e066]    0xfe066: xor    %ax,%ax
[f000:e068]    0xfe068: mov    %ax,%ss
[f000:e06a]    0xfe06a: mov    $0x7000,%esp
[f000:e070]    0xfe070: mov    $0xf431f,%edx
[f000:e076]    0xfe076: jmp    0xfd233
[f000:d233]    0xfd233: mov    %eax,%ecx

# This disables interrupts (CLear Interrupt) so that the current code cannot
# be swapped out by the CPU. It makes sense to do this during critical sections
# of code.
[f000:d236]    0xfd236: cli

# Not sure why the direction flag is being disabled. Direction flag is used by
# the REP instructions.
[f000:d237]    0xfd237: cld
[f000:d238]    0xfd238: mov    $0x8f,%eax

# The next couple of instructions do some I/O. Maybe it's loading the
# bootloader?
[f000:d23e]    0xfd23e: out    %al,$0x70
[f000:d240]    0xfd240: in     $0x71,%al
[f000:d242]    0xfd242: in     $0x92,%al
[f000:d244]    0xfd244: or     $0x2,%al
[f000:d246]    0xfd246: out    %al,$0x92

# lidt (Load Interrupt Descriptor Table). Data structure needed to process
# hardware and software interrupts.
[f000:d248]    0xfd248: lidtw  %cs:0x68f8

# lgdt (Load Global Descriptor Table). Data structure used to hold information
# about the different memory segments code can access.
[f000:d24e]    0xfd24e: lgdtw  %cs:0x68b4

# cr0 is a control register that holds various flags.
# Specifically what the code here does is that it sets the first bit of cr0 to
# 1, which enables protected mode.
[f000:d254]    0xfd254: mov    %cr0,%eax
[f000:d257]    0xfd257: or     $0x1,%eax
[f000:d25b]    0xfd25b: mov    %eax,%cr0

# This instruction switches us to 32-bit mode by changing the code segment.
[f000:d25e]    0xfd25e: ljmpl  $0x8,$0xfd266

# Initialize the segment registers to 0x10. They all now point to the second
# segment descriptor.
0xfd266:     mov    $0x10,%eax
0xfd26b:     mov    %eax,%ds
0xfd26d:     mov    %eax,%es
0xfd26f:     mov    %eax,%ss
0xfd271:     mov    %eax,%fs
0xfd273:     mov    %eax,%gs
0xfd275:     mov    %ecx,%eax

# The BIOS starts calling functions that have no labels so I can't tell what
# they're doing. This is the extent of my BIOS analysis.
0xfd277:     jmp    *%edx
0xf431f:     push   %ebx
0xf4320:     sub    $0x2c,%esp
0xf4323:     movl   $0xf6114,0x4(%esp)
0xf432b:     movl   $0xf5f0f,(%esp)
0xf4332:     call   0xf23f4
...
...
...
```

## Exercise 3

Note that `obj/boot/boot.asm` contains all the assembly executed by the
booloader, both from `boot.S` and `main.c`. The single bootloader binary was
compiled statically so all the code is located in one place.

- At what point does the processor start executing 32-bit code? What exactly
causes the switch from 16- to 32-bit mode?

The switch from 16 bit (real) mode to 32 bit (protected) mode has two
pre-requisites: (1) a global descriptor table needs to be loaded so we can
switch to a code segment that supports 32 bits and (2) we need to enable
protected mode in the CR0 (control 0) register (it's the first bit).

Pre-requisite (1) is done at `0x7c1e: lgdtw  0x7c64` while pre-requisite (2) is
done over the following three instructions:

```
0x7c23:      mov    %cr0,%eax
0x7c26:      or     $0x1,%eax
0x7c2a:      mov    %eax,%cr0
```

The exact instruction which finishes the switch is a long jump to a different
code segment: `0x7c2d: ljmp   $0x8,$0x7c32`. The instruction following this one
will execute in 32-bits.

- What is the last instruction of the boot loader executed, and what is the
first instruction of the kernel it just loaded?

The last instruction executed by the bootloader is a call into the kernel:
`0x7d61:      call   *0x10018`. The address to call into is derived by reading
the ELF header, specifically the `e_entry` field which contains the entry point
of the executable in question.

The first instruction executed by the kernel is:
`0x10000c:    movw   $0x1234,0x472`. This is confirmed by the `kernel.asm` file,
which has `movw   $0x1234,0x472` as the first instruction.

- Where is the first instruction of the kernel?

By 'where' I assume virtual address location. As shown in the previous
question, the start address of the first instruction is `0x10000c`. That somewhat
corresponds to the disassembly of the kernel in `kernel.asm`, where the first
instruction is the same as what I saw in gdb but its address is `0xf010000c`.
Although I think the addresses in the disassembly are link addresses and not
load addresses.

- How does the boot loader decide how many sectors it must read in order to
fetch the entire kernel from disk? Where does it find this information?

The bootloader first fetches the ELF header of the kernel image by reading in
8 sectors. Not sure why it reads in so many, given that the ELF header is not
that large and that later parts of the bootloader also read in the remaining
segments of the file. Anyway, the ELF header contains information on the number
of segments (identified by program headers) contained in the file. The
bootloader code then iterates over all segments and loads each one.

## Exercise 4

Read through the relevant sections of K&R to refresh my pointer arithmetic. I
did have some trouble figuring out that `3[c]` was syntactic sugar for
`*(c + 3)`.

## Exercise 5

The first instruction that would error out is the long jump that switches us
from 16 to 32 bits.

## Exercise 6

When we just enter the bootloader, nothing has yet been loaded in `0x0010000` so
that memory is not guaranteed to contain anything, it's just trash. I presume
it will contain different bits every time the machine starts up.

## Exercise 7

The `0x100025: mov %eax,%cr0` instruction sets flags in the CR0 register,
enabling certain features in the CPU. Looking at `kernel.asm` you can see that
we take whatever was in CR0, OR it with our flags so we don't overwrite what
was already set, and then write that back to CR0. Among other things, we set
the paging enabled flag that turns on the virtual memory translation hardware
unit.

Prior to enabling paging, `x/8x *0x00100000` shows bytes with some values in
them, presumably kernel instructions (in fact if we display that memory as
instructions we get the disassembly of the kernel) while `x/8x *0xf0100000`
shows bytes that are zero-ed out. After we enable paging `x/8x *0xf0100000`
points to the same memory as `0x00100000` because our address is now getting
translated through the memory management unit.

The first instruction that would fail if we did not enable paging would be
the instruction that does a jump to a high virtual address.
```
mov	$relocated, %eax  # relocated = 0xf010002f
jmp	*%eax             # FAIL
```

## Exercise 8

Filled in the octal printer code and verified that it worked by examining the
QEMU printouts during bootup, (6828 base 10 == 15254 base 8).

1. `printf.c` and `console.c` interface via the `cputchar()` function.
`printf.c` wraps `cputchar()` with the `putch()` function that increments
the count argument passed in in addition to calling out to `cputchar()`.
`cputchar()`, as the name suggests is responsible for putting a single character
to the console and that is also the purpose of `putch()`.

2. `crt_pos` is the position of the cursor on the screen. `CRT_SIZE` is the
size of the display (25 columns high and 80 columns wide), or the number of
positions the cursor can take without jumping outside the view of the screen.
Thus, if `crt_pos` > `CRT_SIZE` then the last character written is outside the
current display buffer. The loop shifts back the buffer 80 columns, essentially
printing a new line on the screen.

3. To try out the code we can add it to the `init.c#i386_init()` function.
- `fmt` points to the format string, namely `x %d, y %x, z %d\n`, and `ap`
points to a `va_list`, which presumably stands for variable argument list.
If you print `ap` you can see that it is a pointer, which is
12 bytes higher than the current EBP. If you start printing integers from
`ap` you'll notice that `ap` is in fact pointing to the last of the *variable*
arguments passed to `cprintf()`, from right to left. Below `ap` you'll find a
pointer to the format string.
```
(gdb) p ap
$10 = (va_list) 0xf0110fd4 "\001"
(gdb) p $ebp
$11 = (void *) 0xf0110fc8
(gdb) p/d *(ap) # last of the variable arguments from right to left
$12 = 1
(gdb) p/d *(ap+4)
$13 = 3
(gdb) p/d *(ap+8)
$14 = 4
(gdb) x/a ap-4 # address of fmt string
0xf0110fd0:     0xf0102152
(gdb) x/s 0xf0102152
0xf0102152:     "x %d, y %x, z %d\n"
```

- Here is the order of calls:
```
vcprintf (fmt=0xf0102152 "x %d, y %x, z %d\n", ap=0xf0110fd4 "\001")
cons_putc ('x')
cons_putc (' ')
va_arg () ap before = 0xf0110fd4; ap after = 0xf0110fd8
cons_putc ('1')
cons_putc (',')
cons_putc (' ')
cons_putc ('y')
cons_putc (' ')
va_arg () ap before = 0xf0110fd8; ap after = 0xf0110fdc
cons_putc ('3')
cons_putc (',')
cons_putc (' ')
cons_putc ('z')
cons_putc (' ')
va_arg () ap before = 0xf0110fdc; ap after = 0xf0110fe0
cons_putc ('4')
cons_putc ('\n')
```

- The output is `He110 World`. Note that the format string expects an unsigned
hexadecimal integer as the first format placeholder and a string for the second.
This means that `cprintf()` will format the decimal integer `57616` as hex and
concatenate that to the `H` character. As it turns out, `57616` in hex is
`e110`. For the second format placeholder, we are expecting a string, whose
length is determined by the null character. Note that the `i` variable is
stored in little endian, which means that the order in which the bytes
are read is reversed. Also, note that the `vprintfmt()` function casts the
pointer to the variable to a `char *`. So now the pointer points to the
following sequence of bytes: `0x72 0x6c 0x64 0x00` and if you interpret these
bytes as characters then you get `rld\0`.

If the machine we were executing this on was big endian then the `i` variable
should be changed to `0x726c6400`. On the other hand, the `57616` variable
wouldn't need to change because its value fits in one byte and the order of
bits within a byte is always the same regardless of the endianness of the
machine.

- The value printed will be the 4 bytes that happen to be on the
stack. Those bytes can be an instruction or some other data but because the
format string has two format specifiers, `vprintfmt()` will just interpret
those bytes as an integer.

- The `cprintf()` implementation wouldn't have to change. What you would have
to change are the `va_start`, `va_arg` and `va_end` macros so that they
decrement the pointer everytime they fetch an argument from the stack.

## Exercise 9

The kernel stack is setup in the `entry.S` file, right before jumping into C
code:
```
movl	$0x0,%ebp
movl	$(bootstacktop),%esp
```
`bootstacktop` is a label in `entry.S` to a variable of size `KSTKSIZE`, which
is a typedef to 4 * 4096 bytes. The space is allocated via the assembler
`.space` directive. ESP is set to the top (highest address) end of the reserved
space, namely `0xf0110000`. Therefore the stack starts at `0xf0110000` and
ends at `0xf0110000` - 4 * 4096 = `0xf010c000`. Note that the stack grows
towards lower addresses.

## Exercise 10

Each call to `backtrace()` will contain the following 8 words on the stack:
```
| saved eip address           |
| saved ebp of previous frame | <---- EBP of current frame
| local arg 1                 |
| local arg 2                 |
| local arg 3                 |
| local arg 4                 |
| local arg 5                 |
| local arg 6                 |
```
The local argument words are allocated with the `sub $0x18,%esp` instruction.
Also, the local argument words are used to pass arguments to functions
called within `backtrace()`, including `mon_backtrace()`, `cprintf()` and
recursive calls to `backtrace()`. This is why I did not include a word for the
argument passed to `backtrace()` because technically that word was allocated by
the previous call of `backtrace()`.

## Exercise 11

Initial `mon_backtrace()` implementation done.

## Exercise 12

The kernel linker script, `kernel.ld`, contains code that allocates space for
debugging symbols in the stabs format. The `objdump -h` command further confirms
this theory because it lists two sections `.stab` and `.stabstr`. The `-G` flag
to `objdump` actually lists the contents of the `.stabs` section. Going back
to the bootloader code we can see that it loads the entire kernel image, which
as we just saw contains all the stabs debugging info.

After writing the missing code, the test script (`grade-lab1`) succeeds.
