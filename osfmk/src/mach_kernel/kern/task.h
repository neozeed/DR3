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
 * Revision 2.6.4.3  92/09/15  17:23:01  jeffreyh
 * 	Cleanup for profiling, dont include profil.h
 * 	[92/07/24            bernadat]
 * 
 * Revision 2.6.4.2  92/05/27  00:46:27  jeffreyh
 * 	Added mcmsg_task field to task in MCMSG ifdefs
 * 	[regnier@ssd.intel.com]
 * 
 * Revision 2.6.4.1  92/02/18  19:11:41  jeffreyh
 * 	New fields for profiling
 * 	(Bernard Tabib & Andrei Danes @ gr.osf.org)
 * 	[91/09/16            bernadat]
 * 
 * Revision 2.5.2.1  91/09/26  04:48:34  bernadat
 * 	New fields for profiling
 * 	(Bernard Tabib & Andrei Danes @ gr.osf.org)
 * 	[91/09/16            bernadat]
 * 
 * Revision 2.6  91/11/15  14:11:16  rpd
 * 	NORMA_TASK: added child_node field, which determines where child tasks
 * 	are created for the given task. A value of -1 means local node.
 * 	[91/09/23  09:19:35  jsb]
 * 
 * Revision 2.5  91/05/14  16:48:20  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/02/05  17:30:06  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:19:18  mrt]
 * 
 * Revision 2.3  90/06/02  14:56:48  rpd
 * 	Removed kernel_vm_space, keep_wired.  They are no longer needed.
 * 	[90/04/29            rpd]
 * 	Converted to new IPC.
 * 	[90/03/26  22:23:06  rpd]
 * 
 * Revision 2.2  89/09/08  11:26:47  dbg
 * 	Add 'keep_wired' privilege field, to allow out-of-line data
 * 	passed to task to remain wired.  Needed for default pager.
 * 	Remove kernel_vm_space (not used).
 * 	[89/07/17            dbg]
 * 
 * 19-Oct-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed all non-MACH data structures.
 *
 * Revision 2.6  88/09/25  22:16:41  rpd
 * 	Changed port_cache fields/definitions to obj_cache.
 * 	[88/09/24  18:13:13  rpd]
 * 
 * Revision 2.5  88/08/24  02:46:30  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:24:13  mwyoung]
 * 
 * Revision 2.4  88/07/20  21:07:49  rpd
 * Added ipc_task_lock/ipc_task_unlock definitions.
 * Changes for port sets.
 * Add ipc_next_name field, used for assigning local port names.
 * 
 * Revision 2.3  88/07/17  18:56:33  mwyoung
 * .
 * 
 * Revision 2.2.2.1  88/06/28  20:02:03  mwyoung
 * Cleaned up.  Replaced task_t->kernel_only with
 * task_t->kernel_ipc_space, task_t->kernel_vm_space, and
 * task_t->ipc_privilege, to prevent overloading errors.
 * 
 * Remove current_task() declaration.
 * Eliminate paging_task.
 * 
 * Revision 2.2.1.2  88/06/26  00:45:49  rpd
 * Changes for port sets.
 * 
 * Revision 2.2.1.1  88/06/23  23:32:38  rpd
 * Add ipc_next_name field, used for assigning local port names.
 * 
 * 21-Jun-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Cleaned up.  Replaced task_t->kernel_only with
 *	task_t->kernel_ipc_space, task_t->kernel_vm_space, and
 *	task_t->ipc_privilege, to prevent overloading errors.
 *
 * 19-Apr-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Remove current_task() declaration.
 *	Eliminate paging_task.
 *
 * 18-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed task_data (now is per_thread).  Added
 *	task_bootstrap_port.  Added new routine declarations.
 *	Removed wake_active (unused).  Added fields to accumulate
 *	user and system time for terminated threads.
 *
 *  19-Feb-88 Douglas Orr (dorr) at Carnegie-Mellon University
 *	Change emulation bit mask into vector of routine  addrs
 *
 *  27-Jan-87 Douglas Orr (dorr) at Carnegie-Mellon University
 *	Add support for user space syscall emulation (bit mask
 *	of enabled user space syscalls and user space emulation
 *	routine).
 *
 *  3-Dec-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Change port cache account for per-task port names.
 *	Should move IPC stuff to a separate file :-).
 *	Add reply port for use by kernel-internal tasks.
 *
 *  2-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Added active field.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Eliminate conditionals, flush history.
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *	File:	task.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for tasks.
 *
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 */

#ifndef	_KERN_TASK_H_
#define _KERN_TASK_H_

#include <norma_task.h>
#include <mach_host.h>
#include <fast_tas.h>
#include <task_swapper.h>

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/time_value.h>
#include <mach/message.h>
#include <mach/mach_param.h>
#include <mach/task_info.h>
#include <mach/exception.h>
#include <mach_prof.h>
#include <kern/kern_types.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/syscall_emulation.h>
#include <vm/vm_map.h>
#include <machine/task.h>

typedef struct task {
	/* Synchronization/destruction information */
	decl_mutex_data(,lock)		/* Task's lock */
	int		ref_count;	/* Number of references to me */
	boolean_t	active;		/* Task has not been terminated */
	boolean_t	kernel_loaded;	/* Created with kernel_task_create() */

	/* Miscellaneous */
	vm_map_t	map;		/* Address space description */
	queue_chain_t	pset_tasks;	/* list of tasks assigned to pset */
	void		*user_data;	/* Arbitrary data settable via IPC */
	int		suspend_count;	/* Internal scheduling only */

#if	TASK_SWAPPER
	/* Task swapper data */
	unsigned short	swap_state;	/* swap state (e.g. IN/OUT) */
	unsigned short	swap_flags;	/* swap flags (e.g. MAKE_UNSWAPP) */
	unsigned int	swap_stamp;	/* when last swapped */
	unsigned long	swap_rss;	/* size (pages) when last swapped */
	int		swap_ast_waiting; /* number of threads that have not */
					  /* reached a clean point and halted */
	int		swap_nswap;	/* number of times this task swapped */
	queue_chain_t	swapped_tasks;	/* list of non-resident tasks */
#endif	/* TASK_SWAPPER */

	/* Active activations in this task */
	int		thr_act_count;
	queue_head_t	thr_acts;	/* list of thread_activations */
	int		res_act_count;
	mutex_t		act_list_lock;	/* XXX act_list lock */

	processor_set_t	processor_set;	/* processor set for new threads */
#if	MACH_HOST
	boolean_t	may_assign;	/* can assigned pset be changed? */
	boolean_t	assign_active;	/* waiting for may_assign */
#endif	/* MACH_HOST */

	/* User-visible scheduling information */
	int		user_stop_count;	/* outstanding stops */
	int		priority;		/* for new threads */
	int		max_priority;		/* maximum priority */
	int		sched_data;		/* for use by policy */
	int		policy;			/* scheduling policy */

        /* Task security token */
        security_token_t sec_token;
        
	/* Statistics */
	time_value_t	total_user_time;
				/* total user time for dead threads */
	time_value_t	total_system_time;
				/* total system time for dead threads */

#if	MACH_PROF
	boolean_t	task_profiled;  /* is task being profiled ? */
	struct prof_data *profil_buffer;/* profile struct if so */
#endif	/* MACH_PROF */

	/* IPC structures */
	decl_mutex_data(,itk_lock_data)
	struct ipc_port *itk_self;	/* not a right, doesn't hold ref */
	struct ipc_port *itk_sself;	/* a send right */
	struct exception_action exc_actions[EXC_TYPES_COUNT];
		 			/* a send right each valid element  */
	struct ipc_port *itk_bootstrap;	/* a send right */
	struct ipc_port *itk_registered[TASK_PORT_REGISTER_MAX];
					/* all send rights */

	struct ipc_space *itk_space;

	/* RPC subsystem information */
	queue_head_t	subsystem_list;	/* list of subsystems */
	int		subsystem_count;/* number of subsystems */

	/* Synchronizer ownership information */
	queue_head_t	semaphore_list;		/* list of owned semaphores   */
	queue_head_t	lock_set_list;		/* list of owned lock sets    */
	int		semaphores_owned;	/* number of semaphores owned */
	int 		lock_sets_owned;	/* number of lock sets owned  */

	/* User space system call emulation support */
	struct 	eml_dispatch	*eml_dispatch;

        /* Ledgers */
	struct ipc_port	*wired_ledger_port;
	struct ipc_port *paged_ledger_port;
        
#if	NORMA_TASK
	long		child_node;	/* if != -1, node for new children */
#endif	/* NORMA_TASK */
#if	FAST_TAS
	vm_offset_t	fast_tas_base;
	vm_offset_t	fast_tas_end;
#endif	/* FAST_TAS */
	MACHINE_TASK
} Task;

#define TASK_NULL	((task_t) 0)

typedef	mach_port_t *task_array_t;
typedef	mach_port_t *task_port_array_t;

#define task_lock(task)		mutex_lock(&(task)->lock)
#define task_lock_try(task)	mutex_try(&(task)->lock)
#define task_unlock(task)	mutex_unlock(&(task)->lock)

#define	itk_lock_init(task)	mutex_init(&(task)->itk_lock_data, \
					   ETAP_THREAD_TASK_ITK)
#define	itk_lock(task)		mutex_lock(&(task)->itk_lock_data)
#define	itk_unlock(task)	mutex_unlock(&(task)->itk_lock_data)

/*
 *	Internal only routines
 */

/* Initialize task module */
extern void		task_init(void);

/* task create */
extern kern_return_t	task_create_local(
				task_t		parent_task,
				boolean_t	inherit_memory,
				boolean_t	kernel_loaded,
				task_t		*child_task);	/* OUT */

/* Take reference on task (make sure it doesn't go away) */
extern void		task_reference(
				task_t	task);

/* Remove reference to task */
extern void		task_deallocate(
				task_t	task);

/* Set tasks max priority (does not affect threads within task) */
extern kern_return_t	task_max_priority(
				task_t		task,
				processor_set_t	pset,
				int		max_priority);

#if	MACH_HOST
/* Freeze and unfreeze task from being reassigned processor sets */
extern void		task_freeze(
				task_t		task);

extern void		task_unfreeze(
				task_t		task);
#endif	/* MACH_HOST */

extern void		consider_task_collect(void);

extern task_t		kernel_task;

#endif	/* _KERN_TASK_H_ */
