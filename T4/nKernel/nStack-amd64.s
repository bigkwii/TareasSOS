# Autor: Francisco Cifuentes. Año 2012.
.text
	.align 	8
	.globl	_CallInNewStack
	.globl 	_ChangeToStack
	.globl _ip

# Ojo!! Los argumentos en asm de 64bits se pasan por los
# registros si son de tipo entero o puntero
	
# void* CallInNewStack(int **psp, int *newsp, void (*proc)(), void *ptr)
# rdi = psp
# rsi = newsp
# rdx = proc
# rcx = ptr
_CallInNewStack:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	
#	movq	%rdi, -8(%rbp)
#	movq	%rsi, -16(%rbp)
#	movq	%rdx, -24(%rbp)
#	movq	%rcx, -32(%rbp)
	movq	%rdi, %rax
	movq	%rsp, (%rax) 	# *psp = rsp
	movq	%rsi, %rsp	# rsp = newsp
	movq	%rcx, %rdi	# arg1 = ptr;
	subq  %rbp, %rbp
	call	*%rdx
	ret

# void *ChangeToStack(int **psp, int **pspnew)
# rdi : psp
# rsi : pspnew
# rdx : pret
# no sirve guardarlos...
_ChangeToStack:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	
#	movq	%rdi, -8(%rbp) 	# psp
#	movq	%rsi, -16(%rbp)	# pspnew
#	movq	%rdx, -24(%rbp) # pret
	movq	%rsp, (%rdi)	# *psp = rsp
	movq	%rdi, %rax	# return value= rsp
	movq	(%rsi), %rsp	# rsp = pspnew
#	movq	%rdx, %rax	#
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rbx
	popq	%rbp
	ret
	
_ip:
  movq  (%rsp),%rax
  ret
