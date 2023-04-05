#
# Copyright 1991-1998 by Open Software Foundation, Inc. 
#              All Rights Reserved 
#  
# Permission to use, copy, modify, and distribute this software and 
# its documentation for any purpose and without fee is hereby granted, 
# provided that the above copyright notice appears in all copies and 
# that both the copyright notice and this permission notice appear in 
# supporting documentation. 
#  
# OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE. 
#  
# IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
# LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
# NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
# 
# Mach Operating System
# Copyright (c) 1991,1990 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon
# the rights to redistribute these changes.
#
#
# MkLinux
#

# sun/cthread_inline.awk
#
# Awk script to inline critical C Threads primitives on i386

NF == 2 && $1 == "call" && $2 == "_spin_try_lock" {
	print	"/	BEGIN INLINE spin_try_lock"
	print	"	movl	(%esp),%ecx	/ point at mutex"
	print	"	movl	$1,%eax		/ set locked value in acc"
	print	"	xchg	%eax,(%ecx)	/ locked swap with mutex"
	print	"	xorl	$1,%eax		/ logical complement"
	print	"/	END INLINE spin_try_lock"
	continue
}
NF == 2 && $1 == "call" && $2 == "_spin_unlock" {
	print	"/	BEGIN INLINE " $2
	print	"	movl	(%esp),%ecx"
	print	"	xorl	%eax,%eax	/ set unlocked value in acc"
	print	"	xchg	%eax,(%ecx)	/ locked swap with mutex"
	print	"/	END INLINE " $2
	continue
}
NF == 2 && $1 == "call" && $2 == "_cthread_sp" {
	print	"/	BEGIN INLINE cthread_sp"
	print	"	movl	%esp,%eax"
	print	"/	END INLINE cthread_sp"
	continue
}
# default:
{
	print
}
