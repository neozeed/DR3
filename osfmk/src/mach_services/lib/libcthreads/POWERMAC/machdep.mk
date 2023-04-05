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
# 
# MkLinux

${TARGET_MACHINE}_VPATH		= ${TARGET_MACHINE}
${TARGET_MACHINE}_INCDIRS	= ${TARGET_MACHINE}

SFILES			= csw.o lock.o _setjmp.o
${TARGET_MACHINE}_OFILES = ${SFILES} thread.o

${SFILES:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}

${SFILES}: $${.TARGET:.o=.S}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} '/^#/d' ${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${_CC_} ${_CCFLAGS_} -m601 -Wa,-Z -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}
