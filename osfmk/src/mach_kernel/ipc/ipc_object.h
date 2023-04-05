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
 * Revision 2.6  91/10/09  16:09:27  af
 * 	 Revision 2.5.2.1  91/09/16  10:15:46  rpd
 * 	 	Added (unconditional) ipc_object_print declaration.
 * 	 	[91/09/02            rpd]
 * 
 * Revision 2.5.2.1  91/09/16  10:15:46  rpd
 * 	Added (unconditional) ipc_object_print declaration.
 * 	[91/09/02            rpd]
 * 
 * Revision 2.5  91/05/14  16:35:04  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:22:53  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  15:49:25  mrt]
 * 
 * Revision 2.3  90/11/05  14:29:21  rpd
 * 	Removed ipc_object_reference_macro, ipc_object_release_macro.
 * 	Created new io_reference, io_release macros.
 * 	[90/10/29            rpd]
 * 
 * Revision 2.2  90/06/02  14:51:04  rpd
 * 	Created for new IPC.
 * 	[90/03/26  21:00:05  rpd]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
 */
/*
 *	File:	ipc/ipc_object.h
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Definitions for IPC objects, for which tasks have capabilities.
 */

#ifndef	_IPC_IPC_OBJECT_H_
#define _IPC_IPC_OBJECT_H_

#include <dipc.h>
#include <mach_rt.h>
#include <cpus.h>
#include <mach_kdb.h>

#include <mach/kern_return.h>
#include <mach/message.h>
#include <kern/lock.h>
#include <kern/macro_help.h>
#include <kern/zalloc.h>
#include <ipc/ipc_types.h>
#include <mach/ipc_common.h>	/* for ipc_object data defs */

typedef struct ipc_object *ipc_object_t;

#define	IO_NULL			((ipc_object_t) 0)
#define	IO_DEAD			((ipc_object_t) -1)

#define	IO_VALID(io)		(((io) != IO_NULL) && ((io) != IO_DEAD))

/*
 *	IPC steals the high-order bits from the kotype to use
 *	for its own purposes.  This allows IPC to record facts
 *	about ports that aren't otherwise obvious from the
 *	existing port fields.  In particular, IPC can optionally
 *	mark a port for no more senders detection.  Any change
 *	to IO_BITS_PORT_INFO must be coordinated with bitfield
 *	definitions in ipc_port.h.
 */
#define	IO_BITS_PORT_INFO	0x0000f000	/* stupid port tricks */
#define	IO_BITS_KOTYPE		0x00000fff	/* used by the object */
#define IO_BITS_OTYPE		0x7fff0000	/* determines a zone */
#define	IO_BITS_ACTIVE		0x80000000	/* is object alive? */

#define	io_active(io)		((io)->io_bits & IO_BITS_ACTIVE)
				/* 1.3b26 hacked this to  .. < 0 why? */

#define	io_otype(io)		(((io)->io_bits & IO_BITS_OTYPE) >> 16)
#define	io_kotype(io)		((io)->io_bits & IO_BITS_KOTYPE)

#define	io_makebits(active, otype, kotype)	\
	(((active) ? IO_BITS_ACTIVE : 0) | ((otype) << 16) | (kotype))

/*
 * Object types: ports, port sets, kernel-loaded ports
 */
#define	IOT_PORT		0
#define IOT_PORT_SET		1
#define IOT_NUMBER		2		/* number of types used */

extern zone_t ipc_object_zones[IOT_NUMBER];

#define	io_alloc(otype)		\
		((ipc_object_t) zalloc(ipc_object_zones[(otype)]))

#if	DIPC || MACH_ASSERT
/*
 *	DIPC must intercept the call to deallocate the IPC
 *	object in case the object in question is a port with
 *	a dipc_port extension.
 */
extern void	io_free(
			unsigned int	otype,
			ipc_object_t	object);

#else	/* DIPC || MACH_ASSERT */
#define	io_free(otype, io)	\
		zfree(ipc_object_zones[(otype)], (vm_offset_t) (io))
#endif	/* DIPC || MACH_ASSERT */

/*
 * Here we depend on the ipc_object being first within the ipc_common_data,
 * which is first within the rpc_common_data, which in turn must be first
 * within any kernel data structure needing to lock an ipc_object
 * (ipc_port and ipc_pset).
 */
#if	DIPC && NCPUS == 1

#define io_lock_init(io) \
	usimple_lock_init(&((rpc_common_t)(io))->rcd_io_lock_data,	\
			  ETAP_IPC_OBJECT)
#define	io_lock(io) \
	usimple_lock(&((rpc_common_t)(io))->rcd_io_lock_data)
#define	io_lock_try(io) \
	usimple_lock_try(&((rpc_common_t)(io))->rcd_io_lock_data)
#define	io_unlock(io) \
	usimple_unlock(&((rpc_common_t)(io))->rcd_io_lock_data)

#else	/* DIPC  && NCPUS == 1 */

#define io_lock_init(io) \
	mutex_init(&((rpc_common_t)(io))->rcd_io_lock_data, ETAP_IPC_RPC)
#define	io_lock(io) \
	mutex_lock(&((rpc_common_t)(io))->rcd_io_lock_data)
#define	io_lock_try(io) \
	mutex_try(&((rpc_common_t)(io))->rcd_io_lock_data)
#define	io_unlock(io) \
	mutex_unlock(&((rpc_common_t)(io))->rcd_io_lock_data)

#endif	/* DIPC && NCPUS == 1 */

#if	NCPUS > 1
#define _VOLATILE_ volatile
#else	/* NCPUS > 1 */
#define _VOLATILE_
#endif	/* NCPUS > 1 */

#define io_check_unlock(io) 						\
MACRO_BEGIN								\
	_VOLATILE_ ipc_object_refs_t _refs = (io)->io_references;	\
									\
	io_unlock(io);							\
	if (_refs == 0)							\
		io_free(io_otype(io), io);				\
MACRO_END

/* Sanity check the ref count.  If it is 0, we may be doubly zfreeing.
 * If it is larger than max int, it has been corrupted, probably by being
 * modified into an address (this is architecture dependent, but it's
 * safe to assume there cannot really be max int references).
 *
 * NOTE: The 0 test alone will not catch double zfreeing of ipc_port
 * structs, because the io_references field is the first word of the struct,
 * and zfree modifies that to point to the next free zone element.
 */
#define IO_MAX_REFERENCES						\
	(unsigned)(~0 ^ (1 << (sizeof(int)*BYTE_SIZE - 1)))

#define	io_reference(io)						\
MACRO_BEGIN								\
	assert((io)->io_references < IO_MAX_REFERENCES);		\
	(io)->io_references++;						\
MACRO_END

#define	io_release(io)							\
MACRO_BEGIN								\
	assert((io)->io_references > 0 &&				\
		    (io)->io_references <= IO_MAX_REFERENCES);		\
	(io)->io_references--;						\
MACRO_END

/*
 * Exported interfaces
 */

/* Take a reference to an object */
extern void ipc_object_reference(
	ipc_object_t	object);

/* Release a reference to an object */
extern void ipc_object_release(
	ipc_object_t	object);

/* Look up an object in a space */
extern kern_return_t ipc_object_translate(
	ipc_space_t		space,
	mach_port_t		name,
	mach_port_right_t	right,
	ipc_object_t		*objectp);

/* Allocate a dead-name entry */
extern kern_return_t
ipc_object_alloc_dead(
	ipc_space_t	space,
	mach_port_t 	*namep);

/*  Allocate a dead-name entry, with a specific name */
extern kern_return_t ipc_object_alloc_dead_name(
	ipc_space_t	space,
	mach_port_t	name);

/* Allocate an object */
extern kern_return_t ipc_object_alloc(
	ipc_space_t		space,
	ipc_object_type_t	otype,
	mach_port_type_t	type,
	mach_port_urefs_t	urefs,
	mach_port_t		*namep,
	ipc_object_t		*objectp);

/* Allocate an object, with a specific name */
extern kern_return_t ipc_object_alloc_name(
	ipc_space_t		space,
	ipc_object_type_t	otype,
	mach_port_type_t	type,
	mach_port_urefs_t	urefs,
	mach_port_t		name,
	ipc_object_t		*objectp);

/* Convert a send type name to a received type name */
extern mach_msg_type_name_t ipc_object_copyin_type(
	mach_msg_type_name_t	msgt_name);

/* Copyin a capability from a space */
extern kern_return_t ipc_object_copyin(
	ipc_space_t		space,
	mach_port_t		name,
	mach_msg_type_name_t	msgt_name,
	ipc_object_t		*objectp);

/* Copyin a naked capability from the kernel */
extern void ipc_object_copyin_from_kernel(
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name);

/* Destroy a naked capability */
extern void ipc_object_destroy(
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name);

/* Copyout a capability, placing it into a space */
extern kern_return_t ipc_object_copyout(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	boolean_t		overflow,
	mach_port_t		*namep);

/* Copyout a capability with a name, placing it into a space */
extern kern_return_t ipc_object_copyout_name(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	boolean_t		overflow,
	mach_port_t		name);

/* Translate/consume the destination right of a message */
extern void ipc_object_copyout_dest(
	ipc_space_t		space,
	ipc_object_t		object,
	mach_msg_type_name_t	msgt_name,
	mach_port_t		*namep);

/* Rename an entry in a space */
extern kern_return_t ipc_object_rename(
	ipc_space_t	space,
	mach_port_t	oname,
	mach_port_t	nname);

#if	MACH_RT
/* Determine if an object is real-time */
extern boolean_t ipc_object_is_rt(
	ipc_object_t	object);
#endif	/* MACH_RT */

#if	MACH_KDB
/* Pretty-print an ipc object */

extern void ipc_object_print(
	ipc_object_t	object);

#endif	/* MACH_KDB */

#endif	/* _IPC_IPC_OBJECT_H_ */
