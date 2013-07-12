	.file	"extlibcalls.inv.bc"
	.file	1 "extlibcalls.c"
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
	.globl	main
	.align	16, 0x90
	.type	main,@function
main:
	.cfi_startproc
.Lfunc_begin0:
	.loc	1 6 0
	subq	$360, %rsp
.Ltmp1:
	.cfi_def_cfa_offset 368
	movl	$0, %eax
	leaq	str5, %rcx
	movl	%edi, 304(%rsp)
	movl	%eax, %edi
	movq	%rsi, 296(%rsp)
	movq	%rcx, %rsi
	callq	inv_trace_fn_start
	movl	$1, %edi
	leaq	main, %rcx
	movq	%rcx, %rsi
	callq	recordStartBB
	movl	$1, %edi
	movabsq	$4, %rdx
	leaq	356(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordStore
	movl	$2, %edi
	movabsq	$4, %rdx
	leaq	352(%rsp), %rcx
	movl	$0, 356(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$2, %edi
	leaq	str2, %rdx
	movl	304(%rsp), %esi
	callq	inv_trace_int_value
	movl	$4, %edi
	movabsq	$8, %rdx
	leaq	344(%rsp), %rcx
	movl	304(%rsp), %eax
	movl	%eax, 352(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$7, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rcx
	movq	296(%rsp), %rsi
	movq	%rsi, 344(%rsp)
.Ltmp2:
	movq	%rcx, %rsi
	callq	recordStore
	movl	$9, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rcx
.Ltmp3:
	.loc	1 7 48 prologue_end
	movq	$0, 336(%rsp)
.Ltmp4:
	movq	%rcx, %rsi
	callq	recordStore
	movl	$11, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rcx
.Ltmp5:
	movq	$0, 328(%rsp)
.Ltmp6:
	movq	%rcx, %rsi
	callq	recordStore
	movl	$15, %edi
	movabsq	$4, %rdx
	leaq	316(%rsp), %rcx
.Ltmp7:
	movq	$0, 320(%rsp)
.Ltmp8:
	movq	%rcx, %rsi
	callq	recordStore
	movl	$16, %edi
	movabsq	$4, %rdx
	leaq	312(%rsp), %rcx
.Ltmp9:
	.loc	1 10 3
	movl	$100, 316(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$17, %edi
	movabsq	$4, %rdx
	leaq	308(%rsp), %rcx
	.loc	1 11 3
	movl	$50, 312(%rsp)
	movq	%rcx, %rsi
	callq	recordStore
	movl	$18, %edi
	movabsq	$4, %rdx
	leaq	316(%rsp), %rcx
	.loc	1 12 3
	movl	$150, 308(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$18, %edi
	leaq	str1, %rdx
	.loc	1 14 21
	movl	316(%rsp), %eax
	movl	%eax, %esi
	movl	%eax, 292(%rsp)
	callq	inv_trace_int_value
	movl	292(%rsp), %eax
	movslq	%eax, %rdi
	callq	malloc
	movl	$20, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rcx
	movq	%rcx, %rsi
	movq	%rax, 280(%rsp)
	callq	recordStore
	movl	$21, %edi
	movabsq	$4, %rdx
	leaq	312(%rsp), %rax
	movq	280(%rsp), %rcx
	movq	%rcx, 336(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$21, %edi
	leaq	str1, %rdx
	.loc	1 15 21
	movl	312(%rsp), %r8d
	movl	%r8d, %esi
	movl	%r8d, 276(%rsp)
	callq	inv_trace_int_value
	movl	$22, %edi
	movabsq	$4, %rdx
	leaq	352(%rsp), %rax
	movl	276(%rsp), %esi
	movslq	%esi, %rcx
	movq	%rax, %rsi
	movq	%rcx, 264(%rsp)
	callq	recordLoad
	movl	$22, %edi
	leaq	str1, %rdx
	movl	352(%rsp), %r8d
	movl	%r8d, %esi
	movl	%r8d, 260(%rsp)
	callq	inv_trace_int_value
	movl	260(%rsp), %esi
	addl	$1, %esi
	movslq	%esi, %rax
	movq	264(%rsp), %rdi
	movq	%rax, %rsi
	movq	%rax, 248(%rsp)
	callq	calloc
	movl	$23, %edi
	movq	264(%rsp), %rcx
	movq	248(%rsp), %rdx
	imulq	%rdx, %rcx
	movq	%rax, %rsi
	movq	%rcx, %rdx
	movq	%rax, 240(%rsp)
	callq	recordStore
	movl	$24, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rax
	movq	%rax, %rsi
	callq	recordStore
	movl	$25, %edi
	movabsq	$4, %rdx
	leaq	316(%rsp), %rax
	movq	240(%rsp), %rcx
	movq	%rcx, 328(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$25, %edi
	leaq	str1, %rdx
	.loc	1 16 21
	movl	316(%rsp), %r8d
	movl	%r8d, %esi
	movl	%r8d, 236(%rsp)
	callq	inv_trace_int_value
	movabsq	$2, %rdi
	movl	236(%rsp), %esi
	movslq	%esi, %rax
	movq	%rax, %rsi
	movq	%rax, 224(%rsp)
	callq	calloc
	movl	$26, %edi
	movq	224(%rsp), %rcx
	shlq	$1, %rcx
	movq	%rax, %rsi
	movq	%rcx, %rdx
	movq	%rax, 216(%rsp)
	callq	recordStore
	movl	$27, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rax
	movq	%rax, %rsi
	callq	recordStore
	movl	$28, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rax
	movq	216(%rsp), %rcx
	movq	%rcx, 320(%rsp)
	movq	%rax, %rsi
	callq	recordLoad
	movl	$29, %edi
	movabsq	$4, %rdx
	leaq	316(%rsp), %rax
	.loc	1 17 3
	movq	320(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 208(%rsp)
	callq	recordLoad
	movl	$29, %edi
	leaq	str1, %rdx
	movl	316(%rsp), %r8d
	movl	%r8d, %esi
	movl	%r8d, 204(%rsp)
	callq	inv_trace_int_value
	movl	$30, %edi
	movl	204(%rsp), %esi
	shll	$1, %esi
	movslq	%esi, %rax
	movq	208(%rsp), %rsi
	movq	%rax, %rdx
	movq	%rax, 192(%rsp)
	callq	recordStore
	movl	$0, %esi
	movq	208(%rsp), %rdi
	movq	192(%rsp), %rdx
	callq	memset
	movl	$31, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$32, %edi
	movabsq	$8, %rdx
	leaq	344(%rsp), %rax
	.loc	1 20 3
	movq	336(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 184(%rsp)
	callq	recordLoad
	movl	$33, %edi
	movabsq	$8, %rdx
	movq	344(%rsp), %rax
	movq	%rax, %rcx
	addq	$8, %rcx
	movq	%rcx, %rsi
	movq	%rax, 176(%rsp)
	callq	recordLoad
	movl	$34, %edi
	movq	176(%rsp), %rax
	movq	8(%rax), %rcx
	movq	%rcx, %rsi
	movq	%rcx, 168(%rsp)
	callq	recordStrLoad
	movq	184(%rsp), %rdi
	movq	168(%rsp), %rsi
	callq	strcpy
	movl	$34, %edi
	movq	184(%rsp), %rsi
	movq	%rax, 160(%rsp)
	callq	recordStrStore
	movl	$35, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rax
	movq	%rax, %rsi
	callq	recordLoad
	movl	$36, %edi
	movabsq	$8, %rdx
	leaq	344(%rsp), %rax
	.loc	1 21 3
	movq	336(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 152(%rsp)
	callq	recordLoad
	movl	$37, %edi
	movabsq	$8, %rdx
	movq	344(%rsp), %rax
	movq	%rax, %rcx
	addq	$16, %rcx
	movq	%rcx, %rsi
	movq	%rax, 144(%rsp)
	callq	recordLoad
	movl	$38, %edi
	movq	144(%rsp), %rax
	movq	16(%rax), %rcx
	movq	152(%rsp), %rsi
	movq	%rcx, 136(%rsp)
	callq	recordStrLoad
	movl	$38, %edi
	movq	136(%rsp), %rsi
	callq	recordStrLoad
	movl	$38, %edi
	movq	152(%rsp), %rsi
	movq	136(%rsp), %rdx
	callq	recordStrcatStore
	movq	152(%rsp), %rdi
	movq	136(%rsp), %rsi
	callq	strcat
	movl	$39, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rcx
	movq	%rcx, %rsi
	movq	%rax, 128(%rsp)
	callq	recordLoad
	movl	$40, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rax
	.loc	1 22 3
	movq	328(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 120(%rsp)
	callq	recordLoad
	movl	$41, %edi
	movabsq	$11, %rdx
	movq	336(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 112(%rsp)
	callq	recordLoad
	movl	$41, %edi
	movabsq	$11, %rdx
	movq	120(%rsp), %rsi
	callq	recordStore
	movq	112(%rsp), %rax
	movq	(%rax), %rcx
	movq	120(%rsp), %rdx
	movq	%rcx, (%rdx)
	movw	8(%rax), %r9w
	movw	%r9w, 8(%rdx)
	movb	10(%rax), %r10b
	movb	%r10b, 10(%rdx)
	movl	$42, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$43, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rax
	.loc	1 23 3
	movq	320(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 104(%rsp)
	callq	recordLoad
	movl	$44, %edi
	movabsq	$1, %rdx
	movq	328(%rsp), %rax
	movq	%rax, %rsi
	movq	%rax, 96(%rsp)
	callq	recordLoad
	movl	$44, %edi
	movabsq	$1, %rdx
	movq	104(%rsp), %rsi
	callq	recordStore
	movq	96(%rsp), %rax
	movb	(%rax), %r10b
	movq	104(%rsp), %rcx
	movb	%r10b, (%rcx)
	movl	$45, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rsi
	callq	recordLoad
	movl	$46, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rax
	.loc	1 24 3
	movq	320(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 88(%rsp)
	callq	recordLoad
	movl	$47, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rax
	movq	336(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 80(%rsp)
	callq	recordLoad
	movl	$48, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rax
	movq	328(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 72(%rsp)
	callq	recordLoad
	movl	$49, %edi
	leaq	sprintf, %rax
	movq	320(%rsp), %rcx
	movq	%rax, %rsi
	movq	%rcx, 64(%rsp)
	callq	recordCall
	movl	$49, %edi
	movq	80(%rsp), %rsi
	callq	recordStrLoad
	movl	$49, %edi
	movq	72(%rsp), %rsi
	callq	recordStrLoad
	movl	$49, %edi
	movq	64(%rsp), %rsi
	callq	recordStrLoad
	leaq	.L.str, %rsi
	leaq	str4, %rdx
	movq	88(%rsp), %rdi
	movq	80(%rsp), %rax
	movq	%rdx, 56(%rsp)
	movq	%rax, %rdx
	movq	72(%rsp), %rcx
	movq	64(%rsp), %r8
	movb	$0, %al
	callq	sprintf
	movl	$49, %edi
	movl	%eax, %esi
	movq	56(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$49, %edi
	movq	88(%rsp), %rsi
	callq	recordStrStore
	movl	$49, %edi
	leaq	sprintf, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$50, %edi
	movabsq	$8, %rdx
	leaq	336(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$51, %edi
	movabsq	$8, %rdx
	leaq	328(%rsp), %rcx
	.loc	1 26 3
	movq	336(%rsp), %rsi
	movq	%rsi, 48(%rsp)
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$52, %edi
	leaq	printf, %rcx
	movq	328(%rsp), %rdx
	movq	%rcx, %rsi
	movq	%rdx, 40(%rsp)
	callq	recordCall
	leaq	.L.str1, %rdi
	leaq	str4, %rdx
	movq	48(%rsp), %rsi
	movq	40(%rsp), %rcx
	movq	%rdx, 32(%rsp)
	movq	%rcx, %rdx
	movb	$0, %al
	callq	printf
	movl	$52, %edi
	movl	%eax, %esi
	movq	32(%rsp), %rdx
	callq	inv_trace_int_value
	movl	$52, %edi
	leaq	printf, %rcx
	movq	%rcx, %rsi
	callq	recordReturn
	movl	$53, %edi
	movabsq	$8, %rdx
	leaq	320(%rsp), %rcx
	movq	%rcx, %rsi
	callq	recordLoad
	movl	$54, %edi
	leaq	strlen, %rcx
	.loc	1 30 10
	movq	320(%rsp), %rdx
	movq	%rcx, %rsi
	movq	%rdx, 24(%rsp)
	callq	recordCall
	movl	$54, %edi
	movq	24(%rsp), %rsi
	callq	recordStrLoad
	leaq	str4, %rdx
	movq	24(%rsp), %rdi
	movq	%rdx, 16(%rsp)
	callq	strlen
	movl	$54, %edi
	movq	%rax, %rsi
	movq	16(%rsp), %rdx
	movq	%rax, 8(%rsp)
	callq	inv_trace_long_value
	movl	$54, %edi
	leaq	strlen, %rax
	movq	%rax, %rsi
	callq	recordReturn
	movl	$1, %edi
	leaq	main, %rax
	movq	8(%rsp), %rcx
	movl	%ecx, %r11d
	movl	%edi, 4(%rsp)
	movq	%rax, %rsi
	movl	4(%rsp), %edx
	movl	%r11d, (%rsp)
	callq	recordBB
	movl	$0, %edi
	callq	inv_trace_fn_end
	movl	(%rsp), %eax
	addq	$360, %rsp
	ret
.Ltmp10:
.Ltmp11:
	.size	main, .Ltmp11-main
.Lfunc_end0:
	.cfi_endproc

	.align	16, 0x90
	.type	giriCtor,@function
giriCtor:
	.cfi_startproc
	pushq	%rax
.Ltmp13:
	.cfi_def_cfa_offset 16
	leaq	str6, %rdi
	callq	recordInit
	popq	%rax
	ret
.Ltmp14:
	.size	giriCtor, .Ltmp14-giriCtor
	.cfi_endproc

	.align	16, 0x90
	.type	giriInvGenCtor,@function
giriInvGenCtor:
	.cfi_startproc
	pushq	%rax
.Ltmp16:
	.cfi_def_cfa_offset 16
	leaq	str, %rdi
	callq	inv_invgen_init
	popq	%rax
	ret
.Ltmp17:
	.size	giriInvGenCtor, .Ltmp17-giriInvGenCtor
	.cfi_endproc

	.type	.L.str,@object
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	 "%s %s %s\n"
	.size	.L.str, 10

	.type	.L.str1,@object
.L.str1:
	.asciz	 "%s %s\n"
	.size	.L.str1, 7

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
	.asciz	 "main"
	.size	str5, 5

	.type	str6,@object
str6:
	.asciz	 "bbrecord"
	.size	str6, 9

	.type	removed,@object
	.data
	.globl	removed
	.align	8
removed:
	.long	65535
	.zero	4
	.quad	giriCtor
	.size	removed, 16

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
	.long	230
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
	.byte	5
	.byte	1
	.long	209
	.byte	1
	.quad	.Lfunc_begin0
	.quad	.Lfunc_end0
	.byte	1
	.byte	87
	.byte	1
	.byte	3
	.long	.Lstring5
	.byte	1
	.byte	5
	.long	209
	.byte	3
	.byte	145
	.ascii	 "\340\002"
	.byte	3
	.long	.Lstring6
	.byte	1
	.byte	5
	.long	228
	.byte	3
	.byte	145
	.ascii	 "\330\002"
	.byte	4
	.quad	.Ltmp3
	.quad	.Ltmp10
	.byte	5
	.long	.Lstring8
	.byte	1
	.byte	7
	.long	223
	.byte	3
	.byte	145
	.ascii	 "\320\002"
	.byte	5
	.long	.Lstring9
	.byte	1
	.byte	7
	.long	223
	.byte	3
	.byte	145
	.ascii	 "\310\002"
	.byte	5
	.long	.Lstring10
	.byte	1
	.byte	7
	.long	223
	.byte	3
	.byte	145
	.ascii	 "\300\002"
	.byte	5
	.long	.Lstring11
	.byte	1
	.byte	8
	.long	209
	.byte	3
	.byte	145
	.ascii	 "\274\002"
	.byte	5
	.long	.Lstring12
	.byte	1
	.byte	8
	.long	209
	.byte	3
	.byte	145
	.ascii	 "\270\002"
	.byte	5
	.long	.Lstring13
	.byte	1
	.byte	8
	.long	209
	.byte	3
	.byte	145
	.ascii	 "\264\002"
	.byte	0
	.byte	0
	.byte	6
	.long	.Lstring4
	.byte	5
	.byte	4
	.byte	6
	.long	.Lstring7
	.byte	6
	.byte	1
	.byte	7
	.long	216
	.byte	7
	.long	223
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
	.byte	11
	.byte	1
	.byte	17
	.byte	1
	.byte	18
	.byte	1
	.byte	0
	.byte	0
	.byte	5
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
	.byte	6
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
	.byte	7
	.byte	15
	.byte	0
	.byte	73
	.byte	19
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
	.asciz	 "extlibcalls.c"
.Lstring2:
	.asciz	 "/home/ssahoo2/sw-bug-diagnosis/llvm-3.1/src/projects/diagnosis/test/UnitTests/test1"
.Lstring3:
	.asciz	 "main"
.Lstring4:
	.asciz	 "int"
.Lstring5:
	.asciz	 "argc"
.Lstring6:
	.asciz	 "argv"
.Lstring7:
	.asciz	 "char"
.Lstring8:
	.asciz	 "input1"
.Lstring9:
	.asciz	 "input2"
.Lstring10:
	.asciz	 "input3"
.Lstring11:
	.asciz	 "length1"
.Lstring12:
	.asciz	 "length2"
.Lstring13:
	.asciz	 "length3"

	.section	".note.GNU-stack","",@progbits
