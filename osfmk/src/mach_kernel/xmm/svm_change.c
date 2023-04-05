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
 *	Joseph S. Barrera III, 1992
 *	Routines for implementing distributed temporary objects.
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <kern/misc_protos.h>
#include <kern/xpr.h>
#include <vm/vm_print.h>
#include <xmm/xmm_svm.h>
#include <xmm/xmm_methods.h>

struct change {
	boolean_t	may_cache;
	memory_object_copy_strategy_t
			copy_strategy;
	xmm_reply_data_t reply_data;
	boolean_t	temporary;
#if	MACH_PAGEMAP
	vm_external_map_t existence_map;
	vm_offset_t	existence_size;
#endif	/* MACH_PAGEMAP */
	int		k_count;
	boolean_t	should_apply;
	xmm_obj_t	chosen_kobj;
	vm_size_t	cluster_size;
	change_t	next;
};

zone_t svm_change_zone;

xmm_decl_prototypes(msvm)

/*
 * Forward.
 */
void		svm_send_change_completed(
				xmm_obj_t	mobj,
				change_t	change);

void		svm_queue_change(
				xmm_obj_t	mobj,
				change_t	change,
				boolean_t	should_apply,
				xmm_obj_t	chosen_kobj);

void		svm_apply_change(
				xmm_obj_t	mobj,
				change_t	change);

void		svm_do_change_request(
				xmm_obj_t	mobj);

void
svm_send_change_completed(
	xmm_obj_t	mobj,
	change_t	change)
{
	kern_return_t kr;

	assert(xmm_obj_lock_held(mobj));
	if (change->reply_data.type == XMM_TEMPORARY_REPLY) {
		/*
		 * This request was generated by
		 * svm_disable_active_temporary.
		 * If the object is still alive,
		 * mark the object as temporary_disabled.
		 */
		assert(! MOBJ->temporary_disabled);
		assert(MOBJ->disable_in_progress);
		if (MOBJ->state == MOBJ_STATE_READY) {
			MOBJ->temporary_disabled = TRUE;
		}
		MOBJ->disable_in_progress = FALSE;
		thread_wakeup(svm_disable_event(mobj));
	} else {
		/*
		 * This is a real request.
		 */
		xmm_obj_unlock(mobj);
		kr = M_CHANGE_COMPLETED(mobj, change->may_cache,
					change->copy_strategy,
					&change->reply_data);
		assert(kr == KERN_SUCCESS);
		xmm_obj_lock(mobj);
	}
}

/*
 * MP note : Upon return, the lock has been consumed.
 */
void
svm_queue_change(
	xmm_obj_t	mobj,
	change_t	change,
	boolean_t	should_apply,
	xmm_obj_t	chosen_kobj)
{
	change_t c;

	assert(xmm_obj_lock_held(mobj));

	/*
	 * Complete the initialization of change structure.
	 */
	change->should_apply = should_apply;
	change->chosen_kobj = chosen_kobj;
	change->k_count = 0;
	change->next = 0;

	/*
	 * We need to serialize all changes, including those without a reply.
	 * If there is a change currently being processed, we must queue
	 * our change to be processed later. Otherwise, we process it now.
	 */
	if (MOBJ->change) {
		for (c = MOBJ->change; c->next; c = c->next) {
			continue;
		}
		c->next = change;
		xmm_obj_unlock(mobj);
	} else {
		MOBJ->change = change;
		svm_do_change_request(mobj); /* consumes lock */
	}
}

void
svm_queue_temporary_change(
	xmm_obj_t	kobj)
{
	xmm_obj_t mobj = KOBJ->mobj;
	change_t change;

	/*
	 * Allocate a change structure.
	 */
	change = (change_t) zalloc(svm_change_zone);
	xmm_reply_set(&change->reply_data, XMM_TEMPORARY_REPLY, 0);
	change->may_cache = MOBJ->may_cache;
	change->copy_strategy = MOBJ->copy_strategy;
	change->temporary = FALSE;
#if	MACH_PAGEMAP
	change->existence_map = VM_EXTERNAL_NULL;
	change->existence_size = 0;
#endif	/* MACH_PAGEMAP */

	/*
	 * Queue the change.
	 */
	xmm_obj_lock(mobj);
	svm_queue_change(mobj, change, FALSE, kobj);/* consumes lock */
}

kern_return_t
k_msvm_set_ready(
	xmm_obj_t			mobj,
	boolean_t			may_cache,
	memory_object_copy_strategy_t	copy_strategy,
	vm_size_t			cluster_size,
	xmm_reply_t			reply,
	boolean_t			temporary,
	vm_external_map_t		existence_map,
	vm_offset_t			existence_size)
{
	change_t change;

#ifdef	lint
	(void) K_SET_READY(mobj, may_cache,
			   copy_strategy, cluster_size, reply, temporary,
			   existence_map, existence_size);
#endif	/* lint */
	assert(mobj->class == &msvm_class);
	assert(existence_map == VM_EXTERNAL_NULL);
	assert(existence_size == 0);

	XPR(XPR_XMM,
		"k_msvm_set_ready: mobj %X strat %d emap %X esize %d\n",
		(integer_t)mobj, copy_strategy, (integer_t)existence_map,
		existence_size, 0);
	/*
	 * Record all parameters in a change structure.
	 */
	change = (change_t) zalloc(svm_change_zone);
	change->may_cache = may_cache;
	change->copy_strategy = copy_strategy;
	change->cluster_size = cluster_size;
	if (reply)
		change->reply_data = *reply;
	else
		xmm_reply_set(&change->reply_data, XMM_EMPTY_REPLY, 0);
	change->temporary = temporary;
#if	MACH_PAGEMAP
	change->existence_map = existence_map;
	change->existence_size = existence_size;
#endif	/* MACH_PAGEMAP */

	/*
	 * Queue change, and return.
	 */
	xmm_obj_lock(mobj);
	if (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE ||
	    MOBJ->state == MOBJ_STATE_TERMINATED) {
		xmm_obj_unlock(mobj);
		zfree(svm_change_zone, (vm_offset_t) change);
		return (KERN_INVALID_ARGUMENT);
	}

	svm_queue_change(mobj, change, TRUE, XMM_OBJ_NULL); /* consumes lock */
	return KERN_SUCCESS;
}

kern_return_t
k_msvm_get_attributes(
	xmm_obj_t			mobj,
	memory_object_flavor_t		flavor,
	memory_object_info_t		attributes,
	unsigned			*count)
{
	kern_return_t ret = KERN_SUCCESS;

#ifdef	lint
	(void) K_GET_ATTRIBUTES(mobj, flavor, attributes, count);
#endif	/* lint */

	xmm_obj_lock(mobj);

  	switch (flavor) {
	    case MEMORY_OBJECT_BEHAVIOR_INFO:
	    {
		memory_object_behave_info_t	behave;

		if (*count < MEMORY_OBJECT_BEHAVE_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}
		behave = (memory_object_behave_info_t) attributes;
		behave->copy_strategy = MOBJ->copy_strategy;
		behave->temporary = MOBJ->temporary;
#if notyet	/* remove when vm_msync complies and clean in place fini */
		behave->invalidate = MOBJ->invalidate;
#else
		behave->invalidate = FALSE;
#endif

		*count = MEMORY_OBJECT_BEHAVE_INFO_COUNT;
		break;
	    }
	    case MEMORY_OBJECT_PERFORMANCE_INFO:
	    {
		memory_object_perf_info_t	perf;

		if (*count < MEMORY_OBJECT_PERF_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		perf = (memory_object_perf_info_t) attributes;
		perf->cluster_size = MOBJ->cluster_size;
		perf->may_cache = MOBJ->may_cache;

		*count = MEMORY_OBJECT_PERF_INFO_COUNT;
		break;
	    }
	    case OLD_MEMORY_OBJECT_ATTRIBUTE_INFO:
	    {
		old_memory_object_attr_info_t	attr;

		if (*count < OLD_MEMORY_OBJECT_ATTR_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (old_memory_object_attr_info_t) attributes;
		attr->may_cache = MOBJ->may_cache;
		attr->copy_strategy = MOBJ->copy_strategy;

		*count = OLD_MEMORY_OBJECT_ATTR_INFO_COUNT;
		break;
	    }
	    case MEMORY_OBJECT_ATTRIBUTE_INFO:
	    {
		memory_object_attr_info_t       attr;

		if (*count < MEMORY_OBJECT_ATTR_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (memory_object_attr_info_t) attributes;
		attr->copy_strategy = MOBJ->copy_strategy;
		attr->cluster_size = MOBJ->cluster_size;
		attr->may_cache_object = MOBJ->may_cache;
		attr->temporary = MOBJ->temporary;

		*count = MEMORY_OBJECT_ATTR_INFO_COUNT;
		break;
	    }
	    case MEMORY_OBJECT_NORMA_INFO:
	    {
		memory_object_norma_info_t	norma;

		if (*count < MEMORY_OBJECT_NORMA_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		norma->may_cache = MOBJ->may_cache;
		norma->copy_strategy = MOBJ->copy_strategy;
		norma->cluster_size = MOBJ->cluster_size;

		*count = MEMORY_OBJECT_NORMA_INFO_COUNT;
		break;
	    }

	    default:
		ret = KERN_INVALID_ARGUMENT;
		break;
	}
	xmm_obj_unlock(mobj);
	return(ret);
}

/*
 * Apply change to mobj, but don't contact any kernels.
 */
void
svm_apply_change(
	xmm_obj_t	mobj,
	change_t	change)
{
	assert(xmm_obj_lock_held(mobj));

	/*
	 * Compute our copy strategy based on that of underlying pager.
	 */
	switch (change->copy_strategy) {

	case MEMORY_OBJECT_COPY_NONE:
		/*
		 * We pass this up unchanged. Distributed eager copy
		 * is just like nondistributed eager copy.
		 */
		break;

	case MEMORY_OBJECT_COPY_DELAY:
		/*
		 * We convert this to copy_call. Distributed lazy copy
		 * requires distributed management of the copy object.
		 */
		change->copy_strategy = MEMORY_OBJECT_COPY_CALL;
		break;

	case MEMORY_OBJECT_COPY_SYMMETRIC:
		/*
		 * XXX
		 * We pass this up unchanged, but only for internal objects.
		 * The user xmm layer currently rejects non-internal use of
		 * copy_symmetric. We will probably have to do something
		 * different here when we support non-internal use of
		 * copy_symmetric.
		 */
		break;

#if	MACH_ASSERT
	case MEMORY_OBJECT_COPY_CALL:
	default:
		/*
		 * Illegal or currently unsupported copy strategies
		 * should have been rejected by user xmm layer.
		 */
		change->copy_strategy = MEMORY_OBJECT_COPY_NONE;
		panic("k_msvm_set_ready: bad strategy %d\n",
		      change->copy_strategy);
		break;
#endif	/* MACH_ASSERT */
	}

	/*
	 * For copy-call objects, don't let the default pager override
	 * temporary and cachability attributes, as set upon mobj creation
	 * in svm_create_new_copy.
	 */
	if (MOBJ->shadow) {
		change->may_cache = MOBJ->may_cache;
		change->temporary = MOBJ->temporary;
	}

	/*
	 * XXX
	 * We don't yet support cachable temporary objects.
	 * We thus silently map this combination into cachable non-temporary.
	 * (A mapping into non-cachable temporary would be incorrect, since
	 * this could result in the discarding of required pages. Mapping
	 * into cachable non-temporary errs on the side of doing too much
	 * work, but does not do any real harm.)
	 */
	if (change->temporary && change->may_cache) {
		change->temporary = FALSE;
	}

	/*
	 * MOBJ->temporary_disabled is only valid if mobj is ready and
	 * temporary. If we are just now entering this state, then compute
	 * temporary_disabled. Note that in this case, there are not yet
	 * any active kernels with temporary termination enabled (since
	 * we were either not ready and thus there were no active kernels,
	 * or we were not temporary).
	 */
	if (change->temporary &&
	    ! (MOBJ->temporary && MOBJ->state == MOBJ_STATE_READY)) {
		/*
		 * Compute the number of kernels that are currently
		 * active, or if we have just now become ready, the
		 * number of kernels that will become active.
		 * We must disable temporary termination if there is
		 * expect to be more than one active kernel.
		 */
		if (MOBJ->state == MOBJ_STATE_READY) {
			MOBJ->temporary_disabled =
			    (svm_get_active_count(mobj) > 1);
		} else {
			MOBJ->temporary_disabled =
			    (MOBJ->kobj_count > 1);
		}
	}

	/*
	 * XXX
	 * Now, what about transitioning from temporary to not-temporary?
	 * Should that be allowed? It certainly is chancy, since the object
	 * may be terminated in temporary mode before the change can take
	 * place. It is probably only safe if a change reply is supplied,
	 * and if the termination returns before the change reply, then
	 * the object was probably terminated in temporary mode.
	 *
	 * At any rate, if we are transitioning from temporary to
	 * not-temporary here, do we have to do anything special?
	 */

	MOBJ->copy_strategy = change->copy_strategy;
	MOBJ->may_cache = change->may_cache;
	MOBJ->temporary = change->temporary;

	MOBJ->cluster_size = change->cluster_size;

	/*
	 * XXX
	 * We should have checked earlier for this
	 * object being terminated.
	 */
	assert(MOBJ->state == MOBJ_STATE_CALLED ||
	       MOBJ->state == MOBJ_STATE_READY);

	/*
	 * Set the object ready.
	 * (It may already have been ready.)
	 */
	MOBJ->state = MOBJ_STATE_READY;
	if (MOBJ->ready_wanted) {
		MOBJ->ready_wanted = FALSE;
		thread_wakeup(svm_ready_event(mobj));
	}
}

/*
 * Process change requests associated with this mobj until
 * there are none left or until we must stop to wait for
 * change_completed replies.
 *
 * MP note: Upon return, the lock is consumed.
 */
void
svm_do_change_request(
	xmm_obj_t	mobj)
{
	xmm_obj_t kobj;
	xmm_reply_t reply;
	xmm_reply_data_t reply_data;
	change_t change;
	kern_return_t kr;
	boolean_t release;

	assert(mobj->class == &msvm_class);
	assert(xmm_obj_lock_held(mobj));

	/*
	 * Hold a reference to the mobj to keep it from
	 * being deallocated from underneath us.
	 */
	xmm_obj_reference(mobj);

	for (release = FALSE, change = MOBJ->change;
	     ! release && change;
	     (MOBJ->change = change->next,
	      zfree(svm_change_zone, (vm_offset_t) change),
	      change = MOBJ->change)) {

		/*
		 * Apply the change to the mobj, if appropriate.
		 */
		if (change->should_apply) {
			svm_apply_change(mobj, change);
		}

		/*
		 * If the change request does not have a reply, then simply
		 * deliver the changes to the kernels, and move to the
		 * next change request. In fact, we only need to deliver
		 * the change if the object is now ready.
		 */
		if (xmm_reply_empty(&change->reply_data)) {
			/*
			 * The only place that has a chosen_kobj is
			 * svm_queue_temporary_change, and it uses
			 * XMM_TEMPORARY_REPLY.
			 */
			assert(!change->chosen_kobj);

			if (MOBJ->state != MOBJ_STATE_READY) {
				continue;
			}
			svm_klist_first(mobj, &kobj);
			while (kobj) {
				if (KOBJ->initialized) {
					svm_do_set_ready(kobj,
						XMM_REPLY_NULL);
					xmm_obj_lock(mobj);
					xmm_obj_lock(kobj);
				}
				svm_klist_next(mobj, &kobj, TRUE);
			}
			continue;
		}
		
		/*
		 * We have a reply, which means that we in turn must wait for
		 * replies from every kernel that we notify.
		 *
		 * Increment k_count and decrement when the loop is
		 * exited to make sure we control when that happens; a
		 * nasty race condition exists with change_completed if the
		 * kernels all complete and the count == 0 before we are done.
		 */

		change->k_count++;
		MOBJ->k_count++;
		
		if (change->chosen_kobj) {
			/*
			 * If a kobj was chosen, then it will already have
			 * incremented KOBJ->k_count to prevent this kobj
			 * from terminating while the change is queued.
			 * That k_count reference is donated to this routine
			 * to wait for a reply.
			 */

			kobj = change->chosen_kobj;

			xmm_obj_lock(kobj);
			change->k_count++;
			MOBJ->k_count++;
			
			xmm_obj_reference(mobj);
			xmm_reply_set(&reply_data, XMM_SVM_REPLY, kobj);
			svm_do_set_ready(kobj, &reply_data);
			xmm_obj_lock(mobj);

		} else {
			svm_klist_first(mobj, &kobj);
			while (kobj) {
				if (KOBJ->initialized) {
					change->k_count++;
					MOBJ->k_count++;
					KOBJ->k_count++;

					xmm_obj_reference(mobj);
					xmm_reply_set(&reply_data, XMM_SVM_REPLY, kobj);
					svm_do_set_ready(kobj, &reply_data);
					xmm_obj_lock(mobj);
					xmm_obj_lock(kobj);
				}
				svm_klist_next(mobj, &kobj, TRUE);
			}
		}

		/*
		 * If there are still replies pending for this
		 * change request, then break out of the loop.
		 */
		MOBJ->k_count--;
		if (--change->k_count > 0) {
			break;
		}
		
		/*
		 * This change request has been fully replied to.
		 * Send a change completed message to the memory
		 * manager.
		 *
		 * If the object should terminate, then send
		 * a terminate message before sending the change
		 * completed message.
		 */
		if (release =
		    (MOBJ->k_count == 0 &&
		     MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE &&
		     change->next == CHANGE_NULL)) {
			xmm_obj_unlock(mobj);
			xmm_terminate_pending--;
			MOBJ->term_sent = TRUE;
			kr = M_TERMINATE(mobj, FALSE);	/* m_o_terminate */
			assert(kr == KERN_SUCCESS);
			xmm_obj_lock(mobj);
		}
		svm_send_change_completed(mobj, change);
	}

	/*
	 * Release the mobj reference and finish termination.
	 */
	if (release) {
		xmm_obj_release_quick(mobj);
		xmm_svm_destroy(mobj); /* consumes lock */
	} else
		xmm_obj_release(mobj); /* consumes lock */
}

void
svm_change_completed(
	xmm_obj_t	kobj)
{
	change_t c;
	xmm_obj_t mobj = KOBJ->mobj;
	kern_return_t kr;
	boolean_t release = FALSE;

	xmm_obj_lock(mobj);
	c = MOBJ->change;
	if (c == CHANGE_NULL) {
		xmm_obj_unlock(mobj);
		panic("m_ksvm_change_completed: missing request");
		return;
	}

	xmm_obj_lock(kobj);
	KOBJ->k_count--; /* for this reply */
	if (KOBJ->terminated && (KOBJ->k_count == 0)) {
		xmm_obj_unlock(mobj);
		xmm_svm_terminate(kobj, FALSE);
		xmm_obj_lock(mobj);
	} else
		xmm_obj_unlock(kobj);

	MOBJ->k_count--;
	if (--c->k_count == 0) {

		/*
		 *	If we are supposed to kill the object, then
		 *	do so, making sure that the terminate precedes
		 *	the change_completed.
		 */
		if ((MOBJ->k_count == 0) &&
		    (MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE) &&
		    (c->next == CHANGE_NULL)) {
			MOBJ->term_sent = TRUE;
			xmm_obj_unlock(mobj);
			xmm_terminate_pending--;
			kr = M_TERMINATE(mobj, FALSE);	/* m_o_terminate */
			assert(kr == KERN_SUCCESS);
			release = TRUE;
			xmm_obj_lock(mobj);
		}
		svm_send_change_completed(mobj, c);

		/*
		 *	Free this change and advance list.
		 *	Process remaining change requests, if any.
		 */
		MOBJ->change = c->next;
		zfree(svm_change_zone, (vm_offset_t) c);
		if (MOBJ->change) {
			svm_do_change_request(mobj); /* consumes lock */
			xmm_obj_lock(mobj);
		}
	}

	/*
	 * Release reference obtained by do_change_request() and
	 * 	finish termination.
	 */
	if (release) {
		xmm_obj_release_quick(mobj);
		xmm_svm_destroy(mobj);	/* consumes lock */
	} else
		xmm_obj_release(mobj);	/* consumes lock */
}

void
svm_change_cleanup(
	xmm_obj_t	mobj)
{
	change_t c, c_next;
	xmm_obj_lock(mobj);
	assert(MOBJ->state == MOBJ_STATE_TERMINATED ||
	       MOBJ->state == MOBJ_STATE_SHOULD_TERMINATE);
	c = MOBJ->change;
	xmm_obj_unlock(mobj);

	while (c) {
		c_next = c->next;
		zfree(svm_change_zone, (vm_offset_t) c);
		c = c_next;
	}
}

void
svm_change_init(void)
{
	svm_change_zone =
	    zinit(sizeof(struct change), (vm_size_t) (512*1024),
		  sizeof(struct change), "svm.change");
}

#include <mach_kdb.h>
#if	MACH_KDB
#include	<ddb/db_output.h>

#define	printf	kdbprintf

#define	BOOL(b)		((b) ? "" : "!")

/*
 * Forward.
 */
void	c_svm_print(
		struct change	*change);

/*
 *	Routine:	c_svm_print
 *	Purpose:
 *		Pretty-print an svm change request.
 */

void
c_svm_print(
	struct change	*change)
{
	printf("svm change 0x%x\n", change);

	indent += 2;

	iprintf("may_cache=%d, copy_strategy=%d, ",
		change->may_cache,
		change->copy_strategy);

	printf("reply=0x%x(type ",
		change->reply_data.req);

	switch (change->reply_data.type) {
	case XMM_SVM_REPLY:
		printf("SVM");
		break;

	case XMM_SPLIT_REPLY:
		printf("SPLIT");
		break;

	case XMM_EMPTY_REPLY:
		printf("EMPTY");
		break;

	case XMM_TEMPORARY_REPLY:
		printf("TEMPORARY");
		break;

	default:
		printf("UNKNOWN");
		break;
	}

	printf("), temporary=%d\n", change->temporary);

#if	MACH_PAGEMAP
	iprintf("existence=");
	(void) vm_external_print(change->existence_map, change->existence_size);
#endif	/* MACH_PAGEMAP */

	iprintf("k_count=%d, should_apply=%d, chosen_kobj=0x%x, next=0x%x\n",
		change->k_count,
		change->should_apply,
		change->chosen_kobj,
		change->next);

	indent -=2;
}

#endif	/* MACH_KDB */
