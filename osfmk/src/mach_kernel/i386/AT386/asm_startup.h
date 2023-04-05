/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/* CMU_HIST */
/*
 * Revision 2.3.10.1  92/02/18  18:50:48  jeffreyh
 * 	Support for Corollary MP
 * 	[91/06/25            bernadat]
 * 
 * Revision 2.3  91/05/14  16:19:14  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/08  12:44:00  dbg
 * 	Created (from old i386/start.s).
 * 	[91/04/26  14:40:03  dbg]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * Startup code for an i386 on an AT.
 * Kernel is loaded starting at 1MB.
 * Protected mode, paging disabled.
 */

	popl	%eax
	cmpl	$-1,%eax		/ new calling convention
	je	0f

/*
 * Old calling convention
 *
 * %esp ->	boottype (deprecated)
 *		size of extended memory (K)
 *		size of conventional memory (K)
 *		boothowto (deprecated)
 *		esym (if KDB set up)
 */
#define SYS_REBOOT_COMPAT	1
#if	SYS_REBOOT_COMPAT
	movl	%eax,PA(EXT(boottype))
#endif
	popl	PA(EXT(extmem))		/ extended memory, in K
	popl	PA(EXT(cnvmem))		/ conventional memory, in K
	popl	%edx			/ old boothowto
#if	SYS_REBOOT_COMPAT
#define RB_SINGLE	0x2
#define RB_HALT		0x8
#define RB_ALTBOOT	0x40
	testb	$(RB_SINGLE),%edx	/ old RB_SINGLE flag ?
	je	2f
	incl	PA(EXT(startup_single_user))
2:	testb	$(RB_HALT),%edx		/ old RB_HALT flag ?
	je	2f
	incl	PA(EXT(halt_in_debugger))
2:	testb	$(RB_ALTBOOT),%edx	/ old RB_ALTBOOT flag ?
	je	2f
	incl	PA(EXT(cons_is_com1))
2:	
#if	NCPUS	> 1
	shrl	$0x8,%edx
	movb	%edx,PA(EXT(wncpu))	/ old want ncpus flag
#endif
#endif

	popl	%eax			/ get boot_string & esym
#if	SYS_REBOOT_COMPAT
	movl	%eax, %esi
	lea	PA(EXT(boot_string_store)), %edi
	movl	PA(EXT(boot_string_sz)), %ecx
	cld
	rep
	movsb
#endif

/*
 * Move symbol table out of the way of BSS.
 *
 * When kernel is loaded, at the start of BSS we have:
 * _edata:
 *		.long	kern_sym_size
 *		.long	boot_image_size
 *		.long	load_info_size
 * sym_start:
 *		kernel symbols
 *		.align	ALIGN
 * boot_start:
 *		bootstrap image
 *		.align	ALIGN
 * load_info_start:
 *		bootstrap load information
 *
 * all of which must be moved somewhere else, since it
 * is sitting in the kernel BSS.  In addition, the bootstrap
 * image must be moved to a machine page boundary, so that we get:
 *
 * _edata:
 *		BSS
 * _end:			<-	kern_sym_start (VA)
 *		kernel symbols		. (kern_sym_size)
 * <next page boundary>:	<-	boot_start (VA)
 *		bootstrap image
 *				<-	load_info_start (VA)
 *		load information
 *				<-	%ebx (PA)
 *
 */
	lea	PA(EXT(edata))+4-1,%esi	/ point to symbol size word
	andl	$~0x3,%esi
	movl	(%esi),%edx		/ get symbol size

	lea	PA(EXT(end))+NBPG-1(%edx),%edi
					/ point after BSS, add symbol
					/ size, and round up to
	andl	$-NBPG,%edi		/ machine page boundary

	lea	-KVTOPHYS(%edi),%eax	/ save virtual address
	movl	%eax,PA(EXT(boot_start)) / of start of bootstrap
	movl	4(%esi),%ecx		/ get size of bootstrap
	movl	%ecx,PA(EXT(boot_size))	/ save size of bootstrap
	lea	-KVTOPHYS(%edi,%ecx),%eax
	movl	%eax,PA(EXT(load_info_start))
					/ save virtual address
					/ of start of loader info
	movl	8(%esi),%eax		/ get size of loader info
	movl	%eax,PA(EXT(load_info_size))
					/ save size of loader info
	addl	%eax,%ecx		/ get total size to move

	leal	12(%esi,%edx),%esi	/ point to start of boot image - source

	leal	(%edi,%ecx),%ebx	/ point to new location of
					/ end of bootstrap - next
					/ available physical address

	lea	-4(%esi,%ecx),%esi	/ point to end of src - 4
	lea	-4(%edi,%ecx),%edi	/ point to end of dst - 4
	shrl	$2,%ecx			/ move by longs
	std				/ move backwards
	rep
	movsl				/ move bootstrap and loader_info
	cld				/ reset direction flag

	movl	$EXT(end),PA(EXT(kern_sym_start))
					/ save virtual address
					/ of start of symbols
	movl	%edx,PA(EXT(kern_sym_size)) / save symbol table size
	testl	%edx,%edx		/ any symbols?
	jz	1f			/ if so:

					/ %esi points to start of boot-4
					/ == end of symbol table (source) - 4
	leal	PA(EXT(end))-4(%edx),%edi / point to end of dst - 4
	movl	%edx,%ecx		/ copy size
	shrl	$2,%ecx			/ move by longs
	std				/ move backwards
	rep
	movsl				/ move symbols
	cld				/ reset direction flag

	jmp	1f

/*
 * New calling convention
 *
 * %esp ->	-1
 *		size of extended memory (K)
 *		size of conventional memory (K)
 *		kern_sym_start
 *		kern_sym_size
 *		kern_args_start
 *		kern_args_size
 *		boot_sym_start
 *		boot_sym_size
 *		boot_args_start
 *		boot_args_size
 *		boot_start
 *		boot_size
 *		boot_region_desc
 *		boot_region_count
 *		boot_thread_state_flavor
 *		boot_thread_state
 *		boot_thread_state_count
 *		env_start
 *		env_size
 *		top of loaded memory
 */

#if CBUS
	/*
	 * CBUS shared memory starts at CBUS_START.
	 * The first 16 megs are remapped by H/W at phys address 0
	 * but only on cpu 0 for PC/AT compatibility.
	 * Need to translate phys addresses passed by boot
	 */
#define MEM_BASE CBUS_START
#else /* CBUS */
#define MEM_BASE 0
#endif

#define BOOT_TO_VIRT	(MEM_BASE-(KVTOPHYS))

0:
	popl	PA(EXT(extmem))		/ extended memory, in K
	popl	PA(EXT(cnvmem))		/ conventional memory, in K
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(kern_sym_start))
	popl	PA(EXT(kern_sym_size))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(kern_args_start))
	popl	PA(EXT(kern_args_size))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(boot_sym_start))
	popl	PA(EXT(boot_sym_size))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(boot_args_start))
	popl	PA(EXT(boot_args_size))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(boot_start))
	popl	PA(EXT(boot_size))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(boot_region_desc))
	popl	PA(EXT(boot_region_count))
	popl	PA(EXT(boot_thread_state_flavor))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(boot_thread_state))
	popl	PA(EXT(boot_thread_state_count))
	popl	%eax
	addl	$BOOT_TO_VIRT,%eax	/ convert to virtual address
	movl	%eax,PA(EXT(env_start))
	popl	PA(EXT(env_size))
	popl	%ebx			/ mem top 
	addl	$MEM_BASE,%ebx		/ translate
1:
