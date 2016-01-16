// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
  { "help", "Display this list of commands", mon_help },
  { "kerninfo", "Display information about the kernel", mon_kerninfo },
  { "backtrace", "Display the current stack trace", mon_backtrace },
  { "showmappings", "Display the mappings of the given virtual address range. \
Syntax: showmappings 0xstart 0xend", showmappings },
  { "setperms", "Set the permissions of the given page. Syntax: setperms \
0xvirtualaddr 0xperms", setperms },
  { "singlestep", "Break at the next instruction", single_step },
  { "continue", "Continue execution at the regular rate", continue_exec },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
continue_exec(int argc, char **argv, struct Trapframe *tf)
{
  tf->tf_eflags &= ~FL_TF;
  return -1;
}

int
single_step(int argc, char **argv, struct Trapframe *tf)
{
  tf->tf_eflags |= FL_TF;
  return -1;
}

int
setperms(int argc, char **argv, struct Trapframe *tf)
{
  pte_t *pte;
  pde_t *pde;
  char *end_ptr = argv[1] + strlen(argv[1]) + 1;
  uintptr_t va = (uintptr_t) strtol(argv[1], &end_ptr, 16);
  end_ptr = argv[2] + strlen(argv[2]) + 1;
  uintptr_t perms = (uintptr_t) strtol(argv[2], &end_ptr, 16);

  pte = pgdir_walk(kern_pgdir, (void *) va, false);
  if (!pte) {
    cprintf("Page not mapped.\n");
    return 1;
  }
  perms &= 0xFFF; // ensure perms are only lowest 12 bits
  *pte &= ~0xFFF; // zero out page's permissions
  *pte |= perms; // set new permissions

  pde = &kern_pgdir[PDX(va)]; // now do the same for the page directory entry
  *pde &= ~0xFFF;
  *pde |= perms;
  cprintf("Successfully set permissions on given page.\n");
  return 0;
}

int
showmappings(int argc, char **argv, struct Trapframe *tf)
{
  pte_t *pte;
  pte_t pte_copy;
  physaddr_t page;
  int i;

  char *end_ptr = argv[1] + strlen(argv[1]) + 1;
  uintptr_t current_va = (uintptr_t) strtol(argv[1], &end_ptr, 16);
  end_ptr = argv[2] + strlen(argv[2]) + 1;
  uintptr_t end_va = (uintptr_t) strtol(argv[2], &end_ptr, 16);
  uintptr_t page_size = 0x1000;

  while (current_va <= end_va) {
    pte = pgdir_walk(kern_pgdir, (void *) current_va, false);
    if (!pte || !(*pte & PTE_P)) { 
      cprintf("virtual [%08x] - not mapped\n", current_va);
    } else {
      cprintf("virtual [%08x] - physical [%08x] - perm ",
          current_va, PTE_ADDR(*pte));
      pte_copy = *pte;
      // Couldn't think of a more elegant way to extract perms,
      // feel free to message me if you have a better solution.
      cprintf("[");
      for (i = 0; i < 9; i++) {
        if (pte_copy & PTE_AVAIL) {
          cprintf("AV");
          pte_copy &= ~PTE_AVAIL; // turn that bit off
        } else if (pte_copy & PTE_G) {
          cprintf("G");
          pte_copy &= ~PTE_G;
        } else if (pte_copy & PTE_PS) {
          cprintf("PS");
          pte_copy &= ~PTE_PS;
        } else if (pte_copy & PTE_D) {
          cprintf("D");
          pte_copy &= ~PTE_D;
        } else if (pte_copy & PTE_A) {
          cprintf("A");
          pte_copy &= ~PTE_A;
        } else if (pte_copy & PTE_PCD) {
          cprintf("CD");
          pte_copy &= ~PTE_PCD;
        } else if (pte_copy & PTE_PWT) {
          cprintf("WT");
          pte_copy &= ~PTE_PWT;
        } else if (pte_copy & PTE_U) {
          cprintf("U");
          pte_copy &= ~PTE_U;
        } else if (pte_copy & PTE_W) {
          cprintf("W");
          pte_copy &= ~PTE_W;
        } else {
          cprintf("-");
        }
      }
      cprintf("P"); // we've gotten this far, the page is present
    }
    cprintf("] [%s]\n", convert_to_binary(*pte & 0xFFF));

    current_va += page_size;
  }
  return 0;
}

char *
convert_to_binary(uint32_t raw_binary)
{
  static char *output = "000000000000";
  int i;

  for (i = 11; i >= 0; i--) {
    if (raw_binary & 0x1)
      output[i] = '1';
    else
      output[i] = '0';
    raw_binary = raw_binary >> 1;
  }
  return output;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

  cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  uint8_t max_num_args = 5, i;
	uint32_t current_ebp, prev_ebp, saved_eip;
  uint32_t args[max_num_args];
  char *addr_fmt = "ebp %08x eip %08x args %08x %08x %08x %08x %08x\n";
  char *stack_info = "\t%s:%d: %.*s+%d\n";
  struct Eipdebuginfo eip_info;

  cprintf("Stack backtrace:\n");

  current_ebp = read_ebp();

  while (current_ebp != 0) {
    prev_ebp =  read_byte_at_addr((uint32_t *) current_ebp); 
    saved_eip = read_byte_at_addr((uint32_t *) \
        (current_ebp + 1 * sizeof(uint32_t)));

    for (i = 0; i < max_num_args; i++) {
      args[i] = read_byte_at_addr((uint32_t *) (current_ebp + (i + 2) * \
            sizeof(uint32_t)));
    }

    cprintf(addr_fmt, current_ebp, saved_eip, args[0], args[1], args[2], \
        args[3], args[4]);
    debuginfo_eip(saved_eip, &eip_info);
    cprintf(stack_info, eip_info.eip_file, eip_info.eip_line, \
        eip_info.eip_fn_namelen, eip_info.eip_fn_name, \
        saved_eip - eip_info.eip_fn_addr);

    current_ebp = prev_ebp;
  }

	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
