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
# MkLinux

${TARGET_MACHINE}_VPATH		= ${TARGET_MACHINE}:${MACHINE}

${TARGET_MACHINE}_OFILES	= formats.o elf.o \
				  boot_dep.o set_regs.o

# libgcc.a must be included at end of link line, despite "nostdlib",
# since we need __eabi linkage
# TODO NMGS REMOVE THIS MESS AND CHANGE usr/share/mk/osf.gcc.mk!!


#${TARGET_MACHINE}_LIBS		= ${ELF_CC_EXEC_PREFIX}/libgcc.a

${TARGET_MACHINE}_LDFLAGS	= -T bootstrap_ldscript

bootstrap:	bootstrap_ldscript

bootstrap_ldscript: ${TARGET_MACHINE}/bootstrap_ldscript
	${CP} ${${TARGET_MACHINE}/bootstrap_ldscript:P} ${.TARGET}
