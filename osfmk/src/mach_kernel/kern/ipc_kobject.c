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
 * Revision 2.10.3.4  92/05/27  00:46:06  jeffreyh
 * 	Added ifdef MCMSG for nx_get/put_task_info
 * 	[regnier@ssd.intel.com]
 * 
 * Revision 2.10.3.3  92/03/03  16:20:02  jeffreyh
 * 	Add IKOT_PAGER_TERMINATING case to ipc_kobject_destroy.
 * 	[92/02/25            dlb]
 * 
 * Revision 2.11  92/01/03  20:14:32  dbg
 * 	Call <subsystem>_routine_name to find IPC stub to execute.
 * 	Build reply message header by hand. (XXX)
 * 	Simplify cleanup of request message when reply is success.
 * 	[91/12/18            dbg]
 * 
 * Revision 2.10.3.2  92/02/21  11:23:50  jsb
 * 	Shuffled xmm kobject cases in ipc_kobject_notify.
 * 	[92/02/10  17:26:24  jsb]
 * 
 * Revision 2.10.3.1  92/01/21  21:50:42  jsb
 * 	Removed debugging printf.
 * 	[92/01/21  19:33:50  jsb]
 * 
 * 	Added ipc_kobject_notify for kernel-requested notifications.
 * 	Added code to correctly release send-once kobject destinations.
 * 	[92/01/21  18:20:19  jsb]
 * 
 * Revision 2.10  91/12/13  13:42:24  jsb
 * 	Added support for norma/norma_internal.defs.
 * 
 * Revision 2.9  91/08/01  14:36:18  dbg
 * 	Call machine-dependent interface routine, under
 * 	MACH_MACHINE_ROUTINES.
 * 	[91/08/01            dbg]
 * 
 * Revision 2.8  91/06/17  15:47:02  jsb
 * 	Renamed NORMA conditionals. Added NORMA_VM support.
 * 	[91/06/17  13:46:55  jsb]
 * 
 * Revision 2.7  91/06/06  17:07:05  jsb
 * 	Added NORMA_TASK support.
 * 	[91/05/14  09:05:48  jsb]
 * 
 * Revision 2.6  91/05/18  14:31:42  rpd
 * 	Added check_simple_locks.
 * 	[91/04/01            rpd]
 * 
 * Revision 2.5  91/05/14  16:42:00  mrt
 * 	Correcting copyright
 * 
 * Revision 2.4  91/03/16  14:50:02  rpd
 * 	Replaced ith_saved with ikm_cache.
 * 	[91/02/16            rpd]
 * 
 * Revision 2.3  91/02/05  17:26:37  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:12:51  mrt]
 * 
 * Revision 2.2  90/06/02  14:54:08  rpd
 * 	Created for new IPC.
 * 	[90/03/26  23:46:53  rpd]
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
 *	File:	kern/ipc_kobject.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions for letting a port represent a kernel object.
 */

#include <mach_debug.h>
#include <mach_ipc_test.h>
#include <mach_machine_routines.h>
#include <norma_task.h>
#include <norma_vm.h>
#include <dipc.h>
#include <mach_rt.h>
#include <platforms.h>
#include <xk_proxy.h>

#include <kern/ast.h>
#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/notify.h>
#include <kern/etap_macros.h>
#include <kern/ipc_kobject.h>
#include <kern/misc_protos.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_thread.h>
#include <device/ds_routines.h>
#include <kern/counters.h>

/*
 *	Routine:	ipc_kobject_notify
 *	Purpose:
 *		Deliver notifications to kobjects that care about them.
 */

/* Forward Declarations */

boolean_t
ipc_kobject_notify(
        mach_msg_header_t *request_header,
        mach_msg_header_t *reply_header);

#include <mach/ndr.h>

typedef struct {
        mach_msg_id_t num;
        mig_routine_t routine;
	int size;
#if	MACH_COUNTERS
	mach_counter_t callcount;
#endif
} mig_hash_t;

#define MAX_MIG_ENTRIES 1024
#define MIG_HASH(x) (x)

#ifndef max
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif /* max */

mig_hash_t mig_buckets[MAX_MIG_ENTRIES];
int mig_table_max_displ;
mach_msg_size_t mig_reply_size;

#include <mach/mach_server.h>
#include <mach/mach_port_server.h>
#include <mach/mach_host_server.h>
#include <mach/clock_server.h>
#include <mach/bootstrap_server.h>
#include <mach/ledger_server.h>
#include <mach/sync_server.h>
#include <device/device_server.h>
#include <device/device_pager_server.h>
#if     MACH_DEBUG
#include <mach_debug/mach_debug_server.h>
#endif	/* MACH_DEBUG */
#if     NORMA_TASK
#include <mach/mach_norma_server.h>
#include <kern/norma_task_server.h>
#endif	/* NORMA_TASK */
#if     NORMA_VM
#include <xmm/proxy_server.h>
#include <xmm/xmm_internal_server.h>
#endif	/* NORMA_VM */
#if     MACH_MACHINE_ROUTINES
#include <machine/machine_routines.h>
#endif	/* MACH_MACHINE_ROUTINES */
#if	XK_PROXY
#include <uk_xkern/xk_uproxy_server.h>
#endif	/* XK_PROXY */

rpc_subsystem_t mig_e[] = {
        (rpc_subsystem_t)&mach_subsystem,
        (rpc_subsystem_t)&mach_port_subsystem,
        (rpc_subsystem_t)&mach_host_subsystem,
        (rpc_subsystem_t)&clock_subsystem,
        (rpc_subsystem_t)&do_bootstrap_subsystem,
        (rpc_subsystem_t)&ds_device_subsystem,
        (rpc_subsystem_t)&memory_object_subsystem,
	(rpc_subsystem_t)&sync_subsystem,
	(rpc_subsystem_t)&ledger_subsystem,
#if     MACH_DEBUG
        (rpc_subsystem_t)&mach_debug_subsystem,
#endif  /* MACH_DEBUG */
#if     NORMA_TASK
        (rpc_subsystem_t)&mach_norma_subsystem,
        (rpc_subsystem_t)&norma_task_internal_subsystem,
#endif  /* NORMA_TASK */
#if     NORMA_VM
        (rpc_subsystem_t)&_proxy_subsystem,
        (rpc_subsystem_t)&xmm_internal_subsystem,
#endif  /* NORMA_VM */
#if     XK_PROXY
        (rpc_subsystem_t)&do_uproxy_xk_uproxy_subsystem,
#endif /* XK_PROXY */
#if     MACH_MACHINE_ROUTINES
        (rpc_subsystem_t)&MACHINE_SUBSYSTEM,
#endif  /* MACH_MACHINE_ROUTINES */
#if     MCMSG && iPSC860
	(rpc_subsystem_t)&mcmsg_info_subsystem,
#endif  /* MCMSG && iPSC860 */
};

void
mig_init(void)
{
    register unsigned int i, n = sizeof(mig_e)/sizeof(rpc_subsystem_t);
    register unsigned int howmany;
    register mach_msg_id_t j, pos, nentry, range;
	
    for (i = 0; i < n; i++) {
	range = mig_e[i]->end - mig_e[i]->start;
	if (!mig_e[i]->start || range < 0)
	    panic("the msgh_ids in mig_e[] aren't valid!");
	mig_reply_size = max(mig_reply_size, mig_e[i]->maxsize);

	for  (j = 0; j < range; j++) {
	  if (mig_e[i]->routine[j].stub_routine) { 
	    /* Only put real entries in the table */
	    nentry = j + mig_e[i]->start;	
	    for (pos = MIG_HASH(nentry) % MAX_MIG_ENTRIES, howmany = 1;
		 mig_buckets[pos].num;
		 pos = ++pos % MAX_MIG_ENTRIES, howmany++) {
		    if (mig_buckets[pos].num == nentry)
		 	panic("multiple entries with the same msgh_id");	
		    if (howmany == MAX_MIG_ENTRIES)
		       panic("the mig dispatch table is too small");
	    }
		
	    mig_buckets[pos].num = nentry;
	    mig_buckets[pos].routine = mig_e[i]->routine[j].stub_routine;
	    if (mig_e[i]->routine[j].max_reply_msg)
		    mig_buckets[pos].size = mig_e[i]->routine[j].max_reply_msg;
	    else
		    mig_buckets[pos].size = mig_e[i]->maxsize;

	    mig_table_max_displ = max(howmany, mig_table_max_displ);
	  }
	}
    }
}

/*
 *	Routine:	ipc_kobject_server
 *	Purpose:
 *		Handle a message sent to the kernel.
 *		Generates a reply message.
 *		Version for Untyped IPC.
 *	Conditions:
 *		Nothing locked.
 */

ipc_kmsg_t
ipc_kobject_server(
	ipc_kmsg_t	request)
{
	mach_msg_size_t reply_size;
	ipc_kmsg_t reply;
	kern_return_t kr;
	mig_routine_t routine;
	ipc_port_t *destp;
	mach_msg_format_0_trailer_t *trailer;
	register mig_hash_t *ptr;
#if	MACH_RT
	boolean_t reply_rt;
#endif	/* MACH_RT */
	unsigned int th;

	/* Only fetch current thread if ETAP is configured */
	ETAP_DATA_LOAD(th, current_thread());
        ETAP_PROBE_DATA(ETAP_P_SYSCALL_MACH,
                        EVENT_BEGIN,
			((thread_t) th),
                        &request->ikm_header.msgh_id,
                        sizeof(int));
	/*
         * Find out corresponding mig_hash entry if any
         */
	{
	    register int key = request->ikm_header.msgh_id;
	    register int i = MIG_HASH(key);
	    register int max_iter = mig_table_max_displ;
	
	    do
		ptr = &mig_buckets[i++ % MAX_MIG_ENTRIES];
	    while (key != ptr->num && ptr->num && --max_iter);

	    if (!ptr->routine || key != ptr->num) {
	        ptr = (mig_hash_t *)0;
		reply_size = mig_reply_size;
	    } else {
		reply_size = ptr->size;
#if	MACH_COUNTER
		ptr->callcount++;
#endif
	    }
	}

	/* round up for ikm_cache; overhead is added by ikm_alloc */
        if ((reply_size += MAX_TRAILER_SIZE) < IKM_SAVED_MSG_SIZE)
		reply_size = IKM_SAVED_MSG_SIZE;

#if	MACH_RT
	reply_rt =
	  IP_VALID((ipc_port_t)request->ikm_header.msgh_local_port) ?
	    IP_RT((ipc_port_t)request->ikm_header.msgh_local_port) :
	    FALSE;
	    
	if (reply_rt)
	      reply = ikm_rtalloc(reply_size);
	else
#endif	/* MACH_RT */
	      reply = ikm_alloc(reply_size);

	if (reply == IKM_NULL) {
		printf("ipc_kobject_server: dropping request\n");
		ipc_kmsg_destroy(request);
		return IKM_NULL;
	}

	ikm_init(reply, reply_size);
#if	DIPC
	reply->ikm_handle = HANDLE_NULL;
#endif	/* DIPC */

	/*
	 * Initialize reply message.
	 */
	{
#define	InP	((mach_msg_header_t *) &request->ikm_header)
#define	OutP	((mig_reply_error_t *) &reply->ikm_header)

	    OutP->NDR = NDR_record;
	    OutP->Head.msgh_size = sizeof(mig_reply_error_t);

	    OutP->Head.msgh_bits =
		MACH_MSGH_BITS(MACH_MSGH_BITS_LOCAL(InP->msgh_bits), 0);
	    OutP->Head.msgh_remote_port = InP->msgh_local_port;
	    OutP->Head.msgh_local_port  = MACH_PORT_NULL;
	    OutP->Head.msgh_id = InP->msgh_id + 100;
#if	MACH_RT
	    if (reply_rt)
	          KMSG_MARK_RT(reply);
#endif	/* MACH_RT */
#undef	InP
#undef	OutP
	}

	/*
	 * Find the routine to call, and call it
	 * to perform the kernel function
	 */
	{
	    if (ptr)	
		(*ptr->routine)(&request->ikm_header, &reply->ikm_header);
	    else {
		if (!ipc_kobject_notify(&request->ikm_header, &reply->ikm_header)){
#if	MACH_IPC_TEST
		    printf("ipc_kobject_server: bogus kernel message, id=%d\n",
			request->ikm_header.msgh_id);
#endif	/* MACH_IPC_TEST */
		    ((mig_reply_error_t *) &reply->ikm_header)->RetCode
			= MIG_BAD_ID;
		}
	    }
	}

	/*
	 *	Destroy destination. The following code differs from
	 *	ipc_object_destroy in that we release the send-once
	 *	right instead of generating a send-once notification
	 * 	(which would bring us here again, creating a loop).
	 *	It also differs in that we only expect send or
	 *	send-once rights, never receive rights.
	 *
	 *	We set msgh_remote_port to IP_NULL so that the kmsg
	 *	destroy routines don't try to destroy the port twice.
	 */
	destp = (ipc_port_t *) &request->ikm_header.msgh_remote_port;
	switch (MACH_MSGH_BITS_REMOTE(request->ikm_header.msgh_bits)) {
		case MACH_MSG_TYPE_PORT_SEND:
		    ipc_port_release_send(*destp);
		    break;
		
		case MACH_MSG_TYPE_PORT_SEND_ONCE:
		    ipc_port_release_sonce(*destp);
		    break;
		
		default:
		    panic("ipc_object_destroy: strange destination rights");
	}
	*destp = IP_NULL;

        if (!(reply->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
           ((mig_reply_error_t *) &reply->ikm_header)->RetCode != KERN_SUCCESS)
	 	kr = ((mig_reply_error_t *) &reply->ikm_header)->RetCode;
	else
		kr = KERN_SUCCESS;

	if ((kr == KERN_SUCCESS) || (kr == MIG_NO_REPLY)) {
		/*
		 *	The server function is responsible for the contents
		 *	of the message.  The reply port right is moved
		 *	to the reply message, and we have deallocated
		 *	the destination port right, so we just need
		 *	to free the kmsg.
		 */

		/*
		 * Like ipc_kmsg_put, but without the copyout.  Also,
		 * messages to the kernel will never have been allocated
		 * from the rt_zone.
		 */

		ikm_check_initialized(request, request->ikm_size);
		if (request->ikm_size != IKM_SAVED_KMSG_SIZE ||
		    !ikm_cache_put (request)) {
			ikm_free(request);
		}
	} else {
		/*
		 *	The message contents of the request are intact.
		 *	Destroy everthing except the reply port right,
		 *	which is needed in the reply message.
		 */

		request->ikm_header.msgh_local_port = MACH_PORT_NULL;
		ipc_kmsg_destroy(request);
	}

	if (kr == MIG_NO_REPLY) {
		/*
		 *	The server function will send a reply message
		 *	using the reply port right, which it has saved.
		 */

		ikm_free(reply);

		ETAP_PROBE_DATA(ETAP_P_SYSCALL_MACH,
				EVENT_END,
				((thread_t) th),
				&request->ikm_header.msgh_id,
				sizeof(int));

		return IKM_NULL;
	} else if (!IP_VALID((ipc_port_t)reply->ikm_header.msgh_remote_port)) {
		/*
		 *	Can't queue the reply message if the destination
		 *	(the reply port) isn't valid.
		 */

		ipc_kmsg_destroy(reply);

		ETAP_PROBE_DATA(ETAP_P_SYSCALL_MACH,
				EVENT_END,
				((thread_t) th),
				&request->ikm_header.msgh_id,
				sizeof(int));

		return IKM_NULL;
	}

 	trailer = (mach_msg_format_0_trailer_t *)
		((vm_offset_t)&reply->ikm_header + (int)reply->ikm_header.msgh_size);                
 	trailer->msgh_sender = KERNEL_SECURITY_TOKEN;
 	trailer->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
 	trailer->msgh_trailer_size = MACH_MSG_TRAILER_MINIMUM_SIZE;

        ETAP_PROBE_DATA(ETAP_P_SYSCALL_MACH,
                        EVENT_END,
			((thread_t) th),
                        &request->ikm_header.msgh_id,
                        sizeof(int));

	return reply;
}

/*
 *	Routine:	ipc_kobject_set
 *	Purpose:
 *		Make a port represent a kernel object of the given type.
 *		The caller is responsible for handling refs for the
 *		kernel object, if necessary.
 *	Conditions:
 *		Nothing locked.  The port must be active.
 */

void
ipc_kobject_set(
	ipc_port_t		port,
	ipc_kobject_t		kobject,
	ipc_kobject_type_t	type)
{
	ip_lock(port);
	assert(ip_active(port) || (ip_kotype(port) == IKOT_PAGER_TERMINATING));
#if	MACH_ASSERT
	port->ip_spares[2] = (port->ip_bits & IO_BITS_KOTYPE);
#endif	/* MACH_ASSERT */
	port->ip_bits = (port->ip_bits &~ IO_BITS_KOTYPE) | type;
	port->ip_kobject = kobject;
	ip_unlock(port);
}

/*
 *	Routine:	ipc_kobject_destroy
 *	Purpose:
 *		Release any kernel object resources associated
 *		with the port, which is being destroyed.
 *
 *		This should only be needed when resources are
 *		associated with a user's port.  In the normal case,
 *		when the kernel is the receiver, the code calling
 *		ipc_port_dealloc_kernel should clean up the resources.
 *	Conditions:
 *		The port is not locked, but it is dead.
 */

void
ipc_kobject_destroy(
	ipc_port_t	port)
{
	switch (ip_kotype(port)) {
	    case IKOT_PAGER:
		vm_object_destroy(port);
		break;

	    case IKOT_PAGER_TERMINATING:
		vm_object_pager_wakeup(port);
		break;

	    case IKOT_DEVICE:
	    case IKOT_SEMAPHORE:
	    case IKOT_LOCK_SET:
		break;

	    default:
#if	MACH_ASSERT
		printf("ipc_kobject_destroy: port 0x%x, kobj 0x%x, type %d\n",
		       port, port->ip_kobject, ip_kotype(port));
#endif	/* MACH_ASSERT */
		break;
	}
}

boolean_t
ipc_kobject_notify(
	mach_msg_header_t *request_header,
	mach_msg_header_t *reply_header)
{
	ipc_port_t port = (ipc_port_t) request_header->msgh_remote_port;

	((mig_reply_error_t *) reply_header)->RetCode = MIG_NO_REPLY;
	switch (request_header->msgh_id) {
		case MACH_NOTIFY_PORT_DELETED:
		case MACH_NOTIFY_PORT_DESTROYED:
		case MACH_NOTIFY_NO_SENDERS:
		case MACH_NOTIFY_SEND_ONCE:
		case MACH_NOTIFY_DEAD_NAME:
		break;

		default:
		return FALSE;
	}
	switch (ip_kotype(port)) {
		case IKOT_DEVICE:
		return ds_notify(request_header);

		case IKOT_MASTER_DEVICE:
		return ds_master_notify(request_header);

		default:
                return FALSE;
        }
}

#include <mach_kdb.h>
#if	MACH_COUNTERS && MACH_KDB

#include <ddb/db_output.h>
#include <ddb/db_sym.h>

#define printf  kdbprintf

extern void kobjserver_stats(void);
extern void bucket_stats_print(mig_hash_t *bucket);

extern void kobjserver_stats_clear(void);


void
kobjserver_stats_clear(void)
{
	int i;
	for (i = 0; i < MAX_MIG_ENTRIES; i++) {
		mig_buckets[i].callcount = 0;
	}
}

void
kobjserver_stats(void)
{
    register unsigned int i, n = sizeof(mig_e)/sizeof(rpc_subsystem_t);
    register unsigned int howmany;
    register mach_msg_id_t j, pos, nentry, range;
	
    db_printf("Kobject server call counts:\n");
    for (i = 0; i < n; i++) {
	db_printf("  ");
	db_printsym((vm_offset_t)mig_e[i], DB_STGY_ANY);
	db_printf(":\n");
	range = mig_e[i]->end - mig_e[i]->start;
	if (!mig_e[i]->start || range < 0) continue;

	for  (j = 0; j < range; j++) {
	    nentry = j + mig_e[i]->start;	
	    for (pos = MIG_HASH(nentry) % MAX_MIG_ENTRIES, howmany = 1;
		 mig_buckets[pos].num;
		 pos = ++pos % MAX_MIG_ENTRIES, howmany++) {
		    if (mig_buckets[pos].num == nentry)
			bucket_stats_print(&mig_buckets[pos]);
	    }
	}
    }
}

void
bucket_stats_print(mig_hash_t *bucket)
{
	if (bucket->callcount) {
		db_printf("    ");
		db_printsym((vm_offset_t)bucket->routine, DB_STGY_ANY);
		db_printf(" (%d):\t%d\n", bucket->num, bucket->callcount);
	}
}

#endif	/* MACH_COUNTERS && MACH_KDB */
