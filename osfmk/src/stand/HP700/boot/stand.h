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
 * 
 */
/*
 * MkLinux
 */
/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stand.h	8.1 (Berkeley) 6/11/93
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "saerrno.h"

#define MACH	"/mach"

#ifndef NULL
#define	NULL	0
#endif

extern int errno;

struct open_file;

/*
 * This structure is used to define file system operations in a file system
 * independent way.
 */
struct fs_ops {
	int	(*open) (char *path, struct open_file *f);
	int	(*close) (struct open_file *f);
	int	(*read) (struct open_file *f, char *buf,
			u_int size, u_int *resid);
	int	(*write) (struct open_file *f, char *buf,
			u_int size, u_int *resid);
	off_t	(*seek) (struct open_file *f, off_t offset, int where);
	int	(*stat) (struct open_file *f, struct stat *sb);
};

extern struct fs_ops file_system[];

/* where values for lseek(2) */
#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */

/* Device switch */
struct devsw {
	char	*dv_name;
	int	(*dv_strategy) (void *devdata, int rw,
			daddr_t blk, u_int size, char *buf, u_int *rsize);
	int	(*dv_open)();	/* (struct open_file *f, ...) */
	int	(*dv_close) (struct open_file *f);
	int	(*dv_ioctl) (struct open_file *f, int cmd, void *data);
};

extern struct devsw devsw[];	/* device array */
extern int ndevs;		/* number of elements in devsw[] */

struct open_file {
	int		f_flags;	/* see F_* below */
	struct devsw	*f_dev;		/* pointer to device operations */
	void		*f_devdata;	/* device specific data */
	struct fs_ops	*f_ops;		/* pointer to file system operations */
	void		*f_fsdata;	/* file system specific data */
};

#define	SOPEN_MAX	4
extern struct open_file files[SOPEN_MAX];

/* f_flags values */
#define	F_READ		0x0001	/* file opened for reading */
#define	F_WRITE		0x0002	/* file opened for writing */
#define	F_RAW		0x0004	/* raw device open - no file system */

/*
 * XXX avoid stdio.h which would drag in rune code
 */
#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define tolower(c)	((c) - 'A' + 'a')
#define isspace(c)	((c) == ' ' || (c) == '\t')
#define isdigit(c)	((c) >= '0' && (c) <= '9')

int	devopen (struct open_file *f);
void	*alloc (unsigned size);
void	free (void *ptr, unsigned size);
struct	disklabel;
char	*getdisklabel (const char *buf, struct disklabel *lp);
