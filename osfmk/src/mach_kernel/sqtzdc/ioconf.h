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
/* 
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 Sequent Computer Systems
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON AND SEQUENT COMPUTER SYSTEMS ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND
 * SEQUENT COMPUTER SYSTEMS DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

/* CMU_HIST */
/*
 * Revision 2.3  91/07/31  18:07:56  dbg
 * 	Changed copyright.
 * 	[91/07/31            dbg]
 * 
 * Revision 2.2  91/05/08  13:07:43  dbg
 * 	Added, from Sequent SYMMETRY sources.
 * 	[91/02/26            dbg]
 * 
 */

/*
 * $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987 Sequent Computer Systems, Inc.
 * All rights reserved
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * ZDC configuration structures.
 * 
 * ioconf.h contains structure definitions for items generated by
 * config for the zdc controller.
 */

/*
 *
 * Revision 1.1  89/07/05  13:21:04  kak
 * Initial revision
 * 
 */

#ifndef	_SQTZDC_IOCONF_H_
#define	_SQTZDC_IOCONF_H_

#define	ANY	-1		/* Wildcard */

/*
 * The zdc_conf structure provides the input to drive
 * the autoconfiguration of zdc disk drives.
 */
struct	zdc_conf {
	struct	zdc_driver *zc_driver;		/* -> per-driver data */
	int	zc_nent;			/* # entries in zdc_dev[] */
	struct	zdc_dev    *zc_dev;		/* describes related HW */
};

/*
 * This is a stub for the zdc driver.
 */
struct	zdc_driver {
	char	*zcd_name;		/* name, eg "zd" (no digit) */
};

/*
 * zdc_conf (zc_dev) entry points at array of zdc_dev's; each zdc_dev structure
 * describes a single zdc disk drive.
 *
 * After conf_zdc(), probe_zdc_devices is called to resolve any wildcarding
 * of drives given the following as input.
 */
struct	zdc_dev {
	int 	zdv_idx;		/* zdc index;	-1 == wildcard */
	int 	zdv_drive;		/* drive index; -1 == wildcard */
	int	zdv_drive_type;		/* Drive type index; -1 == wildcard */
};

/*
 * These are generated in ioconf.c by configuration utilities.
 */
#ifdef	MACH_KERNEL
extern	struct	zdc_conf zdc_conf[];
extern	struct	zdc_dev zdc_zd[];
#endif	MACH_KERNEL

#endif	/* _SQTZDC_IOCONF_H_ */