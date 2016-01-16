	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register, which should be caller saved.
 *
 */
	.globl thread_switch
thread_switch:

	# We are running on current_thread's stack, push all registers to save
	pushal

	# Save current thread's stack pointer
	movl current_thread, %eax
	movl %esp, (%eax)

	# Switch stacks to next thread
	movl next_thread, %eax
	movl (%eax), %esp

	# Set current_thread to next_thread (note next_thread is already in eax from above)
	movl %eax, current_thread

	# Set next_thread to 0
	movl $0, next_thread

	# Pop next thread's registers from the stack into the appropriate registers
	popal

	# After poppping all the saved registers from the stack, the sp increased
	# by 32 bytes and now points at the eip of the function passed as an
	# argument in thread_create()
	ret
