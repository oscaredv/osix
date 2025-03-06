/* Switch context from one process to the other by replacing the kernel stack pointer */
.global swtch
swtch:
	movl 4(%esp), %eax /* old_kstack - save the current stack */
	movl 8(%esp), %ecx /* kstack - switch to this stack */

	push %ebp
	push %ebx
	push %esi
	push %edi

	movl %esp, (%eax)
	movl %ecx, %esp

	pop %edi
	pop %esi
	pop %ebx
	pop %ebp
	ret
