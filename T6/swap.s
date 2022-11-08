# int swapInt(int *psl, int val);
.text
	.align	8
	.globl	swapInt
swapInt:
	lock xchgl	(%rdi), %esi
	movl	%esi, %eax
	ret
