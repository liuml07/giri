	.file	"rec-calls.inv.bc"
	.file	1 "rec-calls.c"
	.section	.debug_info,"",@progbits
.Lsection_info:
	.section	.debug_abbrev,"",@progbits
.Lsection_abbrev:
	.section	.debug_aranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_line,"",@progbits
.Lsection_line:
	.section	.debug_loc,"",@progbits
	.section	.debug_pubtypes,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Lsection_str:
	.section	.debug_ranges,"",@progbits
.Ldebug_range:
	.section	.debug_loc,"",@progbits
.Lsection_debug_loc:
	.text
.Ltext_begin:
	.data
	.text
	.globl	increament
	.align	16, 0x90
	.type	increament,@function
increament:
	.cfi_startproc
.Lfunc_begin0:
	.loc	1 9 0
	subq	$24, %rsp
.Ltmp1:
	.cfi_def_cfa_offset 32
	movl	$0, %eax
	leaq	str5, %rsi
	movl	%edi, 16(%rsp)
	movl	%eax, %edi
	callq	inv_trace_fn_start
	movl	$1, %edi
	leaq	increament, %rsi
	callq	recordStartBB
	movl	$1, %edi
	movabsq	$4, %rdx
	leaq	20(%rsp), %rsi
	callq	recordStore
	movl	$1, %edi
	leaq	str2, %rdx
	movl	16(%rsp), %esi
	callq	inv_trace_int_value
	movl	$3, %edi
	movabsq	$4, %rdx
	leaq	20(%rsp), %rcx
	movl	16(%rsp), %eax
	movl	%eax, 20(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$3, %edi
	leaq	str1, %rdx
	.loc	1 10 3 prologue_end
.Ltmp2:
	movl	20(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 12(%rsp)
	callq	inv_trace_int_value
	movl	$1, %eax
	leaq	increament, %rcx
.Ltmp3:
	movl	12(%rsp), %esi
	addl	$1, %esi
	movl	%eax, %edi
	movl	%esi, 8(%rsp)
	movq	%rcx, %rsi
	movl	%eax, %edx
	callq	recordBB
	movl	$0, %edi
	callq	inv_trace_fn_end
	movl	8(%rsp), %eax
	addq	$24, %rsp
	ret
.Ltmp4:
.Ltmp5:
	.size	increament, .Ltmp5-increament
.Lfunc_end0:
	.cfi_endproc

	.globl	add
	.align	16, 0x90
	.type	add,@function
add:
	.cfi_startproc
.Lfunc_begin1:
	.loc	1 14 0
	subq	$40, %rsp
.Ltmp7:
	.cfi_def_cfa_offset 48
	movl	$3, %eax
	leaq	str6, %rcx
	movl	%edi, 28(%rsp)
	movl	%eax, %edi
	movl	%esi, 24(%rsp)
	movq	%rcx, %rsi
	callq	inv_trace_fn_start
	movl	$2, %edi
	leaq	add, %rcx
	movq	%rcx, %rsi
	callq	recordStartBB
	movl	$4, %edi
	movabsq	$4, %rdx
	leaq	36(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$4, %edi
	leaq	str2, %rdx
	movl	28(%rsp), %esi
	callq	inv_trace_int_value
	movl	$6, %edi
	movabsq	$4, %rdx
	leaq	32(%rsp), %rcx
	movl	28(%rsp), %eax
	movl	%eax, 36(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$6, %edi
	leaq	str2, %rdx
	movl	24(%rsp), %esi
	callq	inv_trace_int_value
	movl	$8, %edi
	movabsq	$4, %rdx
	leaq	36(%rsp), %rcx
	movl	24(%rsp), %eax
	movl	%eax, 32(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$8, %edi
	leaq	str1, %rdx
	.loc	1 15 3 prologue_end
.Ltmp8:
	movl	36(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 20(%rsp)
	callq	inv_trace_int_value
	movl	$9, %edi
	movabsq	$4, %rdx
	leaq	32(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$9, %edi
	leaq	str1, %rdx
	movl	32(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 16(%rsp)
	callq	inv_trace_int_value
	movl	$2, %edi
	leaq	add, %rcx
	movl	$1, %edx
	movl	20(%rsp), %eax
	movl	16(%rsp), %esi
	addl	%esi, %eax
	movq	%rcx, %rsi
	movl	%eax, 12(%rsp)
	callq	recordBB
	movl	$3, %edi
	callq	inv_trace_fn_end
	movl	12(%rsp), %eax
	addq	$40, %rsp
	ret
.Ltmp9:
.Ltmp10:
	.size	add, .Ltmp10-add
.Lfunc_end1:
	.cfi_endproc

	.globl	rec_fun
	.align	16, 0x90
	.type	rec_fun,@function
rec_fun:
	.cfi_startproc
.Lfunc_begin2:
	.loc	1 19 0
	subq	$152, %rsp
.Ltmp12:
	.cfi_def_cfa_offset 160
	movl	$9, %eax
	leaq	str7, %rcx
	movq	%rdi, 104(%rsp)
	movl	%eax, %edi
	movq	%rsi, 96(%rsp)
	movq	%rcx, %rsi
	callq	inv_trace_fn_start
	movl	$3, %edi
	leaq	rec_fun, %rcx
	movq	%rcx, %rsi
	callq	recordStartBB
	movl	$10, %edi
	movabsq	$8, %rdx
	leaq	136(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$12, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rcx
	movq	104(%rsp), %rsi
	movq	%rsi, 136(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$17, %edi
	movabsq	$8, %rdx
	leaq	136(%rsp), %rcx
	movq	96(%rsp), %rsi
	movq	%rsi, 128(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$18, %edi
	movabsq	$4, %rdx
	.loc	1 22 3 prologue_end
.Ltmp13:
	movq	136(%rsp), %rcx
	movq	%rcx, %rsi
	movq	%rcx, 88(%rsp)
	callq	recordLoad
	movl	$18, %edi
	leaq	str1, %rdx
	movq	88(%rsp), %rcx
	movl	(%rcx), %eax
	movl	%eax, %esi
	movl	%eax, 84(%rsp)
	callq	inv_trace_int_value
	movl	$19, %edi
	movabsq	$4, %rdx
	leaq	124(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$19, %edi
	leaq	str2, %rdx
	movl	84(%rsp), %esi
	callq	inv_trace_int_value
	movl	$20, %edi
	movabsq	$8, %rdx
	leaq	136(%rsp), %rcx
	movl	84(%rsp), %eax
	movl	%eax, 124(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$21, %edi
	movabsq	$4, %rdx
	.loc	1 24 3
	movq	136(%rsp), %rcx
	movq	%rcx, %rsi
	movq	%rcx, 72(%rsp)
	callq	recordLoad
	movl	$21, %edi
	leaq	str1, %rdx
	movq	72(%rsp), %rcx
	movl	(%rcx), %eax
	movl	%eax, %esi
	movl	%eax, 68(%rsp)
	callq	inv_trace_int_value
	movl	$3, %edi
	leaq	rec_fun, %rcx
	movl	$0, %edx
	movq	%rcx, %rsi
	callq	recordBB
	movl	68(%rsp), %eax
	cmpl	$1, %eax
	jne	.LBB2_2
	movl	$4, %edi
	leaq	rec_fun, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$22, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$23, %edi
	movabsq	$4, %rdx
	.loc	1 25 5
	movq	128(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 56(%rsp)
	callq	recordLoad
	movl	$23, %edi
	leaq	str1, %rdx
	movq	56(%rsp), %rax
	movl	(%rax), %ecx
	movl	%ecx, %esi
	movl	%ecx, 52(%rsp)
	callq	inv_trace_int_value
	movl	$24, %edi
	movabsq	$4, %rdx
	leaq	148(%rsp), %rax
	movq	%rax, %rsi
	callq	recordStore
	movl	$24, %edi
	leaq	str2, %rdx
	movl	52(%rsp), %esi
	callq	inv_trace_int_value
	movl	$4, %edi
	leaq	rec_fun, %rax
	movl	$0, %edx
	movl	52(%rsp), %ecx
	movl	%ecx, 148(%rsp)
	movq	%rax, %rsi
	callq	recordBB
	jmp	.LBB2_3
.LBB2_2:
	movl	$5, %edi
	leaq	rec_fun, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$25, %edi
	movabsq	$4, %rdx
	leaq	116(%rsp), %rax
	movq	%rax, %rsi
	callq	recordStore
	movl	$26, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	.loc	1 27 3
	movl	$1, 116(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$27, %edi
	leaq	rec_fun, %rax
	.loc	1 29 7
	movq	128(%rsp), %rsi
	movq	%rsi, 40(%rsp)
	movq	%rax, %rsi
	callq	recordCall
	leaq	116(%rsp), %rdi
	leaq	str4, %rdx
	movq	40(%rsp), %rsi
	movq	%rdx, 32(%rsp)
	callq	rec_fun
	movl	$27, %edi
	movl	%eax, %esi
	movq	32(%rsp), %rdx
	movl	%eax, 28(%rsp)
	callq	inv_trace_int_value
	movl	$27, %edi
	leaq	rec_fun, %rdx
	movq	%rdx, %rsi
	callq	recordReturn
	movl	$28, %edi
	movabsq	$4, %rdx
	leaq	120(%rsp), %rsi
	callq	recordStore
	movl	$28, %edi
	leaq	str2, %rdx
	movl	28(%rsp), %esi
	callq	inv_trace_int_value
	movl	$29, %edi
	movabsq	$4, %rdx
	leaq	124(%rsp), %rcx
	movl	28(%rsp), %eax
	movl	%eax, 120(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$29, %edi
	leaq	str1, %rdx
	.loc	1 31 3
	movl	124(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 24(%rsp)
	callq	inv_trace_int_value
	movl	$30, %edi
	movabsq	$4, %rdx
	leaq	120(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$30, %edi
	leaq	str1, %rdx
	movl	120(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 20(%rsp)
	callq	inv_trace_int_value
	movl	$31, %edi
	movabsq	$4, %rdx
	leaq	148(%rsp), %rcx
	movl	24(%rsp), %eax
	movl	20(%rsp), %esi
	addl	%esi, %eax
	movq	%rcx, %rsi
	movl	%eax, 16(%rsp)
	callq	recordStore
	movl	$31, %edi
	leaq	str2, %rdx
	movl	16(%rsp), %esi
	callq	inv_trace_int_value
	movl	$5, %edi
	leaq	rec_fun, %rcx
	movl	$0, %edx
	movl	16(%rsp), %eax
	movl	%eax, 148(%rsp)
	movq	%rcx, %rsi
	callq	recordBB
.LBB2_3:
	movl	$6, %edi
	leaq	rec_fun, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$32, %edi
	movabsq	$4, %rdx
	leaq	148(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$32, %edi
	leaq	str1, %rdx
	.loc	1 32 1
	movl	148(%rsp), %ecx
	movl	%ecx, %esi
	movl	%ecx, 12(%rsp)
	callq	inv_trace_int_value
	movl	$6, %edi
	leaq	rec_fun, %rax
	movl	$1, %edx
	movq	%rax, %rsi
	callq	recordBB
	movl	$9, %edi
	callq	inv_trace_fn_end
	movl	12(%rsp), %eax
	addq	$152, %rsp
	ret
.Ltmp14:
.Ltmp15:
	.size	rec_fun, .Ltmp15-rec_fun
.Lfunc_end2:
	.cfi_endproc

	.globl	main
	.align	16, 0x90
	.type	main,@function
main:
	.cfi_startproc
.Lfunc_begin3:
	.loc	1 35 0
	subq	$120, %rsp
.Ltmp17:
	.cfi_def_cfa_offset 128
	movl	$32, %eax
	leaq	str8, %rcx
	movl	%edi, 92(%rsp)
	movl	%eax, %edi
	movq	%rsi, 80(%rsp)
	movq	%rcx, %rsi
	callq	inv_trace_fn_start
	movl	$7, %edi
	leaq	main, %rcx
	movq	%rcx, %rsi
	callq	recordStartBB
	movl	$33, %edi
	movabsq	$4, %rdx
	leaq	116(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$34, %edi
	movabsq	$4, %rdx
	leaq	112(%rsp), %rcx
	movl	$0, 116(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$34, %edi
	leaq	str2, %rdx
	movl	92(%rsp), %esi
	callq	inv_trace_int_value
	movl	$36, %edi
	movabsq	$8, %rdx
	leaq	104(%rsp), %rcx
	movl	92(%rsp), %eax
	movl	%eax, 112(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$40, %edi
	movabsq	$8, %rdx
	leaq	104(%rsp), %rcx
	movq	80(%rsp), %rsi
	movq	%rsi, 104(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$41, %edi
	movabsq	$8, %rdx
	.loc	1 38 7 prologue_end
.Ltmp18:
	movq	104(%rsp), %rcx
.Ltmp19:
	movq	%rcx, %rsi
	addq	$8, %rsi
	movq	%rcx, 72(%rsp)
	callq	recordLoad
	movl	$42, %edi
	leaq	atoi, %rcx
	movq	72(%rsp), %rdx
	movq	8(%rdx), %rsi
	movq	%rsi, 64(%rsp)
	movq	%rcx, %rsi
	callq	recordCall
	leaq	str4, %rdx
	movq	64(%rsp), %rdi
	movq	%rdx, 56(%rsp)
	callq	atoi
	movl	$42, %edi
	movl	%eax, %esi
	movq	56(%rsp), %rdx
	movl	%eax, 52(%rsp)
	callq	inv_trace_int_value
	movl	$42, %edi
	leaq	atoi, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$43, %edi
	movabsq	$4, %rdx
	leaq	100(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$43, %edi
	leaq	str2, %rdx
	movl	52(%rsp), %esi
	callq	inv_trace_int_value
	movl	$44, %edi
	movabsq	$8, %rdx
	leaq	104(%rsp), %rcx
	movl	52(%rsp), %eax
	movl	%eax, 100(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$45, %edi
	movabsq	$8, %rdx
	.loc	1 39 7
	movq	104(%rsp), %rcx
	movq	%rcx, %rsi
	addq	$16, %rsi
	movq	%rcx, 40(%rsp)
	callq	recordLoad
	movl	$46, %edi
	leaq	atoi, %rcx
	movq	40(%rsp), %rdx
	movq	16(%rdx), %rsi
	movq	%rsi, 32(%rsp)
	movq	%rcx, %rsi
	callq	recordCall
	leaq	str4, %rdx
	movq	32(%rsp), %rdi
	movq	%rdx, 24(%rsp)
	callq	atoi
	movl	$46, %edi
	movl	%eax, %esi
	movq	24(%rsp), %rdx
	movl	%eax, 20(%rsp)
	callq	inv_trace_int_value
	movl	$46, %edi
	leaq	atoi, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$47, %edi
	movabsq	$4, %rdx
	leaq	96(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$47, %edi
	leaq	str2, %rdx
	movl	20(%rsp), %esi
	callq	inv_trace_int_value
	movl	$48, %edi
	leaq	rec_fun, %rcx
	movl	20(%rsp), %eax
	movl	%eax, 96(%rsp)
	movq	%rcx, %rsi
	callq	recordCall
	leaq	100(%rsp), %rdi
	leaq	96(%rsp), %rsi
	leaq	str4, %rdx
	.loc	1 40 10
	movq	%rdx, 8(%rsp)
	callq	rec_fun
	movl	$48, %edi
	movl	%eax, %esi
	movq	8(%rsp), %rdx
	movl	%eax, 4(%rsp)
	callq	inv_trace_int_value
	movl	$48, %edi
	leaq	rec_fun, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$7, %edi
	leaq	main, %rcx
	movl	$1, %edx
	movq	%rcx, %rsi
	callq	recordBB
	movl	$32, %edi
	callq	inv_trace_fn_end
	movl	4(%rsp), %eax
	addq	$120, %rsp
	ret
.Ltmp20:
.Ltmp21:
	.size	main, .Ltmp21-main
.Lfunc_end3:
	.cfi_endproc

	.globl	PrintHello
	.align	16, 0x90
	.type	PrintHello,@function
PrintHello:
	.cfi_startproc
.Lfunc_begin4:
	.loc	1 63 0
	subq	$72, %rsp
.Ltmp23:
	.cfi_def_cfa_offset 80
	leaq	str12, %rax
	movq	%rdi, 40(%rsp)
	movq	%rax, %rdi
	callq	recordHandlerThreadID
	movl	$48, %edi
	leaq	str9, %rsi
	callq	inv_trace_fn_start
	movl	$8, %edi
	leaq	PrintHello, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$49, %edi
	movabsq	$8, %rdx
	leaq	56(%rsp), %rax
	movq	%rax, %rsi
	callq	recordStore
	movl	$52, %edi
	movabsq	$8, %rdx
	leaq	56(%rsp), %rax
	movq	40(%rsp), %rsi
	movq	%rsi, 56(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$53, %edi
	movabsq	$8, %rdx
	leaq	48(%rsp), %rax
.Ltmp24:
	.loc	1 65 4 prologue_end
	movq	56(%rsp), %rsi
	movq	%rsi, 32(%rsp)
	movq	%rax, %rsi
	callq	recordStore
	movl	$53, %edi
	leaq	str2, %rdx
	movq	32(%rsp), %rsi
	callq	inv_trace_long_value
	movl	$54, %edi
	movabsq	$8, %rdx
	leaq	48(%rsp), %rax
	movq	32(%rsp), %rsi
	movq	%rsi, 48(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$54, %edi
	leaq	str1, %rdx
	.loc	1 66 4
	movq	48(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 24(%rsp)
	callq	inv_trace_long_value
	movl	$55, %edi
	leaq	pthread_self, %rax
	movq	%rax, %rsi
	callq	recordCall
	leaq	str4, %rdx
	.loc	1 66 75
	movq	%rdx, 16(%rsp)
	callq	pthread_self
	movl	$55, %edi
	movq	%rax, %rsi
	movq	16(%rsp), %rdx
	movq	%rax, 8(%rsp)
	callq	inv_trace_long_value
	movl	$55, %edi
	leaq	pthread_self, %rax
	movq	%rax, %rsi
	callq	recordReturn
	movl	$56, %edi
	leaq	printf, %rax
	movq	%rax, %rsi
	callq	recordCall
	leaq	.L.str, %rdi
	leaq	str4, %rdx
	movq	24(%rsp), %rsi
	movq	8(%rsp), %rax
	movq	%rdx, (%rsp)
	movq	%rax, %rdx
	movb	$0, %al
	callq	printf
	movl	$56, %edi
	movl	%eax, %esi
	movq	(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$56, %edi
	leaq	printf, %rdx
	movq	%rdx, %rsi
	callq	recordReturn
	movabsq	$0, %rdi
	.loc	1 67 4
	callq	pthread_exit
	movl	$8, %edi
	leaq	PrintHello, %rdx
	movl	$0, %eax
	movq	%rdx, %rsi
	movl	%eax, %edx
	callq	recordBB
.Ltmp25:
.Ltmp26:
	.size	PrintHello, .Ltmp26-PrintHello
.Lfunc_end4:
	.cfi_endproc

	.globl	threadCreator
	.align	16, 0x90
	.type	threadCreator,@function
threadCreator:
	.cfi_startproc
.Lfunc_begin5:
	.loc	1 71 0
	subq	$168, %rsp
.Ltmp28:
	.cfi_def_cfa_offset 176
	movl	$56, %edi
	leaq	str10, %rsi
	callq	inv_trace_fn_start
	movl	$10, %edi
	leaq	threadCreator, %rsi
	callq	recordStartBB
	movl	$62, %edi
	leaq	pthread_self, %rsi
	callq	recordCall
	leaq	str4, %rdx
	.loc	1 76 47 prologue_end
.Ltmp29:
	movq	%rdx, 120(%rsp)
	callq	pthread_self
	movl	$62, %edi
	movq	%rax, %rsi
	movq	120(%rsp), %rdx
	movq	%rax, 112(%rsp)
	callq	inv_trace_long_value
	movl	$62, %edi
	leaq	pthread_self, %rax
	movq	%rax, %rsi
	callq	recordReturn
	movl	$63, %edi
	leaq	printf, %rax
	movq	%rax, %rsi
	callq	recordCall
	leaq	.L.str1, %rdi
	leaq	str4, %rdx
	movq	112(%rsp), %rsi
	movb	$0, %al
	movq	%rdx, 104(%rsp)
	callq	printf
	movl	$63, %edi
	movl	%eax, %esi
	movq	104(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$63, %edi
	leaq	printf, %rdx
	movq	%rdx, %rsi
	callq	recordReturn
	movl	$64, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rsi
	callq	recordStore
	movl	$10, %edi
	leaq	threadCreator, %rdx
	movl	$0, %eax
	.loc	1 78 8
.Ltmp30:
	movq	$0, 128(%rsp)
	movq	%rdx, %rsi
	movl	%eax, %edx
	callq	recordBB
.LBB5_1:
	movl	$11, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$65, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$65, %edi
	leaq	str1, %rdx
	movq	128(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 96(%rsp)
	callq	inv_trace_long_value
	movl	$11, %edi
	leaq	threadCreator, %rax
	movl	$0, %edx
	movq	%rax, %rsi
	callq	recordBB
	movq	96(%rsp), %rax
	cmpq	$2, %rax
	jge	.LBB5_6
	movl	$12, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$66, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$66, %edi
	leaq	str1, %rdx
	.loc	1 79 7
.Ltmp31:
	movq	128(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 88(%rsp)
	callq	inv_trace_long_value
	movl	$67, %edi
	leaq	printf, %rax
	movq	%rax, %rsi
	callq	recordCall
	leaq	.L.str2, %rdi
	leaq	str4, %rdx
	movq	88(%rsp), %rsi
	movb	$0, %al
	movq	%rdx, 80(%rsp)
	callq	printf
	movl	$67, %edi
	movl	%eax, %esi
	movq	80(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$67, %edi
	leaq	printf, %rdx
	movq	%rdx, %rsi
	callq	recordReturn
	movl	$68, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rsi
	callq	recordLoad
	movl	$68, %edi
	leaq	str1, %rdx
	.loc	1 80 12
	movq	128(%rsp), %rsi
	movq	%rsi, 72(%rsp)
	callq	inv_trace_long_value
	movl	$69, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rsi
	leaq	144(%rsp), %rcx
	movq	72(%rsp), %r8
	shlq	$3, %r8
	addq	%r8, %rcx
	movq	%rcx, 64(%rsp)
	callq	recordLoad
	movl	$69, %edi
	leaq	str1, %rdx
	movq	128(%rsp), %rcx
	movq	%rcx, %rsi
	movq	%rcx, 56(%rsp)
	callq	inv_trace_long_value
	movl	$70, %edi
	leaq	pthread_create, %rcx
	movq	%rcx, %rsi
	callq	recordExtCall
	movl	$70, %edi
	leaq	PrintHello, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movabsq	$0, %rsi
	leaq	PrintHello, %rdx
	leaq	str4, %rcx
	movq	64(%rsp), %rdi
	movq	56(%rsp), %r8
	movq	%rcx, 48(%rsp)
	movq	%r8, %rcx
	callq	pthread_create
	movl	$70, %edi
	movl	%eax, %esi
	movq	48(%rsp), %rdx
	movl	%eax, 44(%rsp)
	callq	inv_trace_int_value
	movl	$70, %edi
	leaq	PrintHello, %rcx
	movq	%rcx, %rsi
	callq	recordCall
	movl	$70, %edi
	leaq	pthread_create, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$71, %edi
	movabsq	$4, %rdx
	leaq	140(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$71, %edi
	leaq	str2, %rdx
	movl	44(%rsp), %esi
	callq	inv_trace_int_value
	movl	$72, %edi
	movabsq	$4, %rdx
	leaq	140(%rsp), %rcx
	movl	44(%rsp), %eax
	movl	%eax, 140(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$72, %edi
	leaq	str1, %rdx
	.loc	1 81 7
	movl	140(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 40(%rsp)
	callq	inv_trace_int_value
	movl	$12, %edi
	leaq	threadCreator, %rcx
	movl	$0, %edx
	movq	%rcx, %rsi
	callq	recordBB
	movl	40(%rsp), %eax
	cmpl	$0, %eax
	je	.LBB5_4
	movl	$13, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$73, %edi
	movabsq	$4, %rdx
	leaq	140(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$73, %edi
	leaq	str1, %rdx
	.loc	1 82 10
.Ltmp32:
	movl	140(%rsp), %ecx
	movl	%ecx, %esi
	movl	%ecx, 36(%rsp)
	callq	inv_trace_int_value
	movl	$74, %edi
	leaq	printf, %rax
	movq	%rax, %rsi
	callq	recordCall
	leaq	.L.str3, %rdi
	leaq	str4, %rdx
	movl	36(%rsp), %esi
	movb	$0, %al
	movq	%rdx, 24(%rsp)
	callq	printf
	movl	$74, %edi
	movl	%eax, %esi
	movq	24(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$74, %edi
	leaq	printf, %rdx
	movq	%rdx, %rsi
	callq	recordReturn
	movl	$4294967295, %edi
	.loc	1 83 10
	callq	exit
	movl	$13, %edi
	leaq	threadCreator, %rdx
	movl	$0, %eax
	movq	%rdx, %rsi
	movl	%eax, %edx
	callq	recordBB
	jmp	.LBB5_7
.Ltmp33:
.LBB5_4:
	movl	$14, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$14, %edi
	leaq	threadCreator, %rax
	movl	$0, %edx
	movq	%rax, %rsi
	callq	recordBB
	movl	$15, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$76, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$76, %edi
	leaq	str1, %rdx
	.loc	1 78 28
	movq	128(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 16(%rsp)
	callq	inv_trace_long_value
	movl	$77, %edi
	movabsq	$8, %rdx
	leaq	128(%rsp), %rax
	movq	16(%rsp), %rsi
	addq	$1, %rsi
	movq	%rsi, 8(%rsp)
	movq	%rax, %rsi
	callq	recordStore
	movl	$77, %edi
	leaq	str2, %rdx
	movq	8(%rsp), %rsi
	callq	inv_trace_long_value
	movl	$15, %edi
	leaq	threadCreator, %rax
	movl	$0, %edx
	movq	8(%rsp), %rsi
	movq	%rsi, 128(%rsp)
	movq	%rax, %rsi
	callq	recordBB
	jmp	.LBB5_1
.Ltmp34:
.LBB5_6:
	movl	$16, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movabsq	$0, %rdi
	.loc	1 86 4
	callq	pthread_exit
	movl	$16, %edi
	leaq	threadCreator, %rax
	movl	$0, %edx
	movq	%rax, %rsi
	callq	recordBB
.Ltmp35:
.LBB5_7:
	movl	$18, %edi
	leaq	threadCreator, %rax
	movq	%rax, %rsi
	callq	recordStartBB
	movl	$18, %edi
	leaq	threadCreator, %rax
	movl	$0, %edx
	movq	%rax, %rsi
	callq	recordBB
.Ltmp36:
	.size	threadCreator, .Ltmp36-threadCreator
.Lfunc_end5:
	.cfi_endproc

	.align	16, 0x90
	.type	giriCtor,@function
giriCtor:
	.cfi_startproc
	pushq	%rax
.Ltmp38:
	.cfi_def_cfa_offset 16
	leaq	str11, %rdi
	callq	recordInit
	popq	%rax
	ret
.Ltmp39:
	.size	giriCtor, .Ltmp39-giriCtor
	.cfi_endproc

	.align	16, 0x90
	.type	giriInvGenCtor,@function
giriInvGenCtor:
	.cfi_startproc
	pushq	%rax
.Ltmp41:
	.cfi_def_cfa_offset 16
	leaq	str, %rdi
	callq	inv_invgen_init
	popq	%rax
	ret
.Ltmp42:
	.size	giriInvGenCtor, .Ltmp42-giriInvGenCtor
	.cfi_endproc

	.type	.L.str,@object
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	 "Hello World! It's me, thread #%ld! pthread_self #%lu!\n"
	.size	.L.str, 55

	.type	.L.str1,@object
.L.str1:
	.asciz	 "MAIN thread pthread_self #%lu!\n"
	.size	.L.str1, 32

	.type	.L.str2,@object
.L.str2:
	.asciz	 "In main: creating thread %ld\n"
	.size	.L.str2, 30

	.type	.L.str3,@object
.L.str3:
	.asciz	 "ERROR; return code from pthread_create() is %d\n"
	.size	.L.str3, 48

	.type	str,@object
	.section	.rodata,"a",@progbits
str:
	.asciz	 "ddgtrace.out"
	.size	str, 13

	.type	str1,@object
str1:
	.asciz	 "load"
	.size	str1, 5

	.type	str2,@object
str2:
	.asciz	 "store"
	.size	str2, 6

	.type	str3,@object
str3:
	.asciz	 "ret"
	.size	str3, 4

	.type	str4,@object
str4:
	.asciz	 "fncall"
	.size	str4, 7

	.type	str5,@object
str5:
	.asciz	 "increament"
	.size	str5, 11

	.type	str6,@object
str6:
	.asciz	 "add"
	.size	str6, 4

	.type	str7,@object
str7:
	.asciz	 "rec_fun"
	.size	str7, 8

	.type	str8,@object
str8:
	.asciz	 "main"
	.size	str8, 5

	.type	str9,@object
str9:
	.asciz	 "PrintHello"
	.size	str9, 11

	.type	str10,@object
str10:
	.asciz	 "threadCreator"
	.size	str10, 14

	.type	str11,@object
str11:
	.asciz	 "bbrecord"
	.size	str11, 9

	.type	removed,@object
	.data
	.globl	removed
	.align	8
removed:
	.long	65535
	.zero	4
	.quad	giriCtor
	.size	removed, 16

	.type	str12,@object
	.section	.rodata,"a",@progbits
str12:
	.asciz	 "PrintHello"
	.size	str12, 11

	.section	.ctors,"aw",@progbits
	.align	8
	.quad	giriCtor
	.quad	giriInvGenCtor
	.text
.Ltext_end:
	.data
.Ldata_end:
	.text
.Lsection_end1:
	.section	.debug_info,"",@progbits
.Linfo_begin1:
	.long	624
	.short	2
	.long	.Labbrev_begin
	.byte	8
	.byte	1
	.long	.Lstring0
	.short	12
	.long	.Lstring1
	.quad	0
	.long	.Lsection_line
	.long	.Lstring2
	.byte	2
	.long	.Lstring3
	.byte	1
	.byte	8
	.byte	1
	.long	85
	.byte	1
	.quad	.Lfunc_begin0
	.quad	.Lfunc_end0
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring10
	.byte	1
	.byte	8
	.long	85
	.byte	2
	.byte	145
	.byte	20
	.byte	0
	.byte	4
	.long	.Lstring4
	.byte	5
	.byte	4
	.byte	2
	.long	.Lstring5
	.byte	1
	.byte	13
	.byte	1
	.long	85
	.byte	1
	.quad	.Lfunc_begin1
	.quad	.Lfunc_end1
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring11
	.byte	1
	.byte	13
	.long	85
	.byte	2
	.byte	145
	.byte	36
	.byte	3
	.long	.Lstring12
	.byte	1
	.byte	13
	.long	85
	.byte	2
	.byte	145
	.byte	32
	.byte	0
	.byte	2
	.long	.Lstring6
	.byte	1
	.byte	18
	.byte	1
	.long	85
	.byte	1
	.quad	.Lfunc_begin2
	.quad	.Lfunc_end2
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring11
	.byte	1
	.byte	18
	.long	565
	.byte	3
	.byte	145
	.ascii	 "\210\001"
	.byte	3
	.long	.Lstring12
	.byte	1
	.byte	18
	.long	565
	.byte	3
	.byte	145
	.ascii	 "\200\001"
	.byte	5
	.quad	.Ltmp13
	.quad	.Ltmp14
	.byte	6
	.long	.Lstring13
	.byte	1
	.byte	20
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\374"
	.byte	6
	.long	.Lstring14
	.byte	1
	.byte	20
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\370"
	.byte	6
	.long	.Lstring15
	.byte	1
	.byte	20
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\364"
	.byte	0
	.byte	0
	.byte	2
	.long	.Lstring7
	.byte	1
	.byte	34
	.byte	1
	.long	85
	.byte	1
	.quad	.Lfunc_begin3
	.quad	.Lfunc_end3
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring16
	.byte	1
	.byte	34
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\360"
	.byte	3
	.long	.Lstring17
	.byte	1
	.byte	34
	.long	582
	.byte	3
	.byte	145
	.asciz	 "\350"
	.byte	5
	.quad	.Ltmp18
	.quad	.Ltmp20
	.byte	6
	.long	.Lstring13
	.byte	1
	.byte	36
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\344"
	.byte	6
	.long	.Lstring14
	.byte	1
	.byte	36
	.long	85
	.byte	3
	.byte	145
	.asciz	 "\340"
	.byte	0
	.byte	0
	.byte	2
	.long	.Lstring8
	.byte	1
	.byte	62
	.byte	1
	.long	469
	.byte	1
	.quad	.Lfunc_begin4
	.quad	.Lfunc_end4
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring19
	.byte	1
	.byte	62
	.long	469
	.byte	2
	.byte	145
	.byte	56
	.byte	5
	.quad	.Ltmp24
	.quad	.Ltmp25
	.byte	6
	.long	.Lstring20
	.byte	1
	.byte	64
	.long	587
	.byte	2
	.byte	145
	.byte	48
	.byte	0
	.byte	0
	.byte	7
	.byte	8
	.long	.Lstring9
	.byte	1
	.byte	70
	.long	85
	.byte	1
	.quad	.Lfunc_begin5
	.quad	.Lfunc_end5
	.byte	1
	.byte	87
	.byte	1
	.byte	5
	.quad	.Ltmp29
	.quad	.Ltmp35
	.byte	6
	.long	.Lstring22
	.byte	1
	.byte	72
	.long	615
	.byte	3
	.byte	145
	.ascii	 "\220\001"
	.byte	6
	.long	.Lstring25
	.byte	1
	.byte	73
	.long	85
	.byte	3
	.byte	145
	.ascii	 "\214\001"
	.byte	6
	.long	.Lstring26
	.byte	1
	.byte	74
	.long	587
	.byte	3
	.byte	145
	.ascii	 "\200\001"
	.byte	0
	.byte	0
	.byte	9
	.long	85
	.byte	4
	.long	.Lstring18
	.byte	6
	.byte	1
	.byte	9
	.long	570
	.byte	9
	.long	577
	.byte	4
	.long	.Lstring21
	.byte	5
	.byte	8
	.byte	4
	.long	.Lstring23
	.byte	7
	.byte	8
	.byte	10
	.long	594
	.long	.Lstring24
	.byte	1
	.byte	50
	.byte	11
	.byte	4
	.byte	5
	.byte	12
	.long	601
	.byte	13
	.long	612
	.byte	1
	.byte	0
	.byte	0
.Linfo_end1:
	.section	.debug_abbrev,"",@progbits
.Labbrev_begin:
	.byte	1
	.byte	17
	.byte	1
	.byte	37
	.byte	14
	.byte	19
	.byte	5
	.byte	3
	.byte	14
	.byte	17
	.byte	1
	.byte	16
	.byte	6
	.byte	27
	.byte	14
	.byte	0
	.byte	0
	.byte	2
	.byte	46
	.byte	1
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	39
	.byte	12
	.byte	73
	.byte	19
	.byte	63
	.byte	12
	.byte	17
	.byte	1
	.byte	18
	.byte	1
	.byte	64
	.byte	10
	.ascii	 "\347\177"
	.byte	12
	.byte	0
	.byte	0
	.byte	3
	.byte	5
	.byte	0
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	73
	.byte	19
	.byte	2
	.byte	10
	.byte	0
	.byte	0
	.byte	4
	.byte	36
	.byte	0
	.byte	3
	.byte	14
	.byte	62
	.byte	11
	.byte	11
	.byte	11
	.byte	0
	.byte	0
	.byte	5
	.byte	11
	.byte	1
	.byte	17
	.byte	1
	.byte	18
	.byte	1
	.byte	0
	.byte	0
	.byte	6
	.byte	52
	.byte	0
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	73
	.byte	19
	.byte	2
	.byte	10
	.byte	0
	.byte	0
	.byte	7
	.byte	15
	.byte	0
	.byte	0
	.byte	0
	.byte	8
	.byte	46
	.byte	1
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	73
	.byte	19
	.byte	63
	.byte	12
	.byte	17
	.byte	1
	.byte	18
	.byte	1
	.byte	64
	.byte	10
	.ascii	 "\347\177"
	.byte	12
	.byte	0
	.byte	0
	.byte	9
	.byte	15
	.byte	0
	.byte	73
	.byte	19
	.byte	0
	.byte	0
	.byte	10
	.byte	22
	.byte	0
	.byte	73
	.byte	19
	.byte	3
	.byte	14
	.byte	58
	.byte	11
	.byte	59
	.byte	11
	.byte	0
	.byte	0
	.byte	11
	.byte	36
	.byte	0
	.byte	11
	.byte	11
	.byte	62
	.byte	11
	.byte	0
	.byte	0
	.byte	12
	.byte	1
	.byte	1
	.byte	73
	.byte	19
	.byte	0
	.byte	0
	.byte	13
	.byte	33
	.byte	0
	.byte	73
	.byte	19
	.byte	47
	.byte	11
	.byte	0
	.byte	0
	.byte	0
.Labbrev_end:
	.section	.debug_pubtypes,"",@progbits
.Lset0 = .Lpubtypes_end1-.Lpubtypes_begin1
	.long	.Lset0
.Lpubtypes_begin1:
	.short	2
	.long	.Linfo_begin1
.Lset1 = .Linfo_end1-.Linfo_begin1
	.long	.Lset1
	.long	0
.Lpubtypes_end1:
	.section	.debug_aranges,"",@progbits
	.section	.debug_ranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Lstring0:
	.asciz	 "clang version 3.1 (branches/release_31)"
.Lstring1:
	.asciz	 "rec-calls.c"
.Lstring2:
	.asciz	 "/home/ssahoo2/sw-bug-diagnosis/llvm-3.1/src/projects/diagnosis/test/UnitTests/test3"
.Lstring3:
	.asciz	 "increament"
.Lstring4:
	.asciz	 "int"
.Lstring5:
	.asciz	 "add"
.Lstring6:
	.asciz	 "rec_fun"
.Lstring7:
	.asciz	 "main"
.Lstring8:
	.asciz	 "PrintHello"
.Lstring9:
	.asciz	 "threadCreator"
.Lstring10:
	.asciz	 "i"
.Lstring11:
	.asciz	 "a"
.Lstring12:
	.asciz	 "b"
.Lstring13:
	.asciz	 "x"
.Lstring14:
	.asciz	 "y"
.Lstring15:
	.asciz	 "z"
.Lstring16:
	.asciz	 "argc"
.Lstring17:
	.asciz	 "argv"
.Lstring18:
	.asciz	 "char"
.Lstring19:
	.asciz	 "threadid"
.Lstring20:
	.asciz	 "tid"
.Lstring21:
	.asciz	 "long int"
.Lstring22:
	.asciz	 "threads"
.Lstring23:
	.asciz	 "long unsigned int"
.Lstring24:
	.asciz	 "pthread_t"
.Lstring25:
	.asciz	 "rc"
.Lstring26:
	.asciz	 "t"

	.section	".note.GNU-stack","",@progbits
