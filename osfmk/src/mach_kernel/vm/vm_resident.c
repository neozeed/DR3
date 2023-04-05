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
 * Revision 2.20.3.1  92/03/03  16:27:21  jeffreyh
 * 	Changes from TRUNK
 * 	[92/02/26  12:36:11  jeffreyh]
 * 
 * Revision 2.21  92/01/14  16:48:32  rpd
 * 	Changed vm_page_info for CountInOut.
 * 	[92/01/14            rpd]
 * 
 * Revision 2.20  91/10/09  16:20:54  af
 * 	Added vm_page_deactivate_behind.
 * 	[91/10/05            rpd]
 * 
 * Revision 2.19  91/08/28  11:19:00  jsb
 * 	Added vm_page_free_count_minimum.
 * 	Fixed divergence between vm_page_free and vm_page_replace.
 * 	Fixed vm_page_deactivate to handle absent/fictitious pages properly.
 * 	[91/08/07            rpd]
 * 	Replaced divergent, expanded vm_page_free code in vm_page_replace
 * 	with a call to vm_page_free itself.
 * 	[91/08/15  18:49:59  jsb]
 * 
 * Revision 2.18  91/07/31  18:22:15  dbg
 * 	Redefine 'private' to mean private memory, not private page
 * 	structure.  Calling vm_page_free on a private page frees the
 * 	page structure but not the page.
 * 	[91/07/30  17:27:50  dbg]
 * 
 * Revision 2.17  91/07/01  08:28:24  jsb
 * 	Removed accidently merged hack.
 * 	[91/06/29  17:47:21  jsb]
 * 
 * 	20-Jun-91 David L. Black (dlb) at Open Software Foundation
 * 	Need vm_page_replace in all configurations.
 * 	[91/06/29  16:37:31  jsb]
 * 
 * Revision 2.16  91/06/20  07:33:45  rvb
 * 	Add pmap_page_grap_phys_addr() so that we don't have to
 *	export vm_page_t.
 * 
 * Revision 2.15  91/06/17  15:49:43  jsb
 * 	Renamed NORMA conditionals. Fixed vm_page_rename implementation.
 * 	[91/06/17  11:25:16  jsb]
 * 
 * Revision 2.14  91/06/06  17:08:43  jsb
 * 	NORMA_IPC: added vm_page_replace.
 * 	[91/05/14  09:40:19  jsb]
 * 
 * Revision 2.13  91/05/18  14:42:01  rpd
 * 	Renamed vm_page_fictitious_zone to vm_page_zone.
 * 	[91/05/16            rpd]
 * 
 * 	Moved deactivate-behind code from vm_page_alloc to vm_page_insert.
 * 	[91/04/21            rpd]
 * 
 * 	Fixed vm_page_deactivate as suggested by rfr,
 * 	to clear the reference bit on inactive/referenced pages.
 * 	[91/04/20            rpd]
 * 
 * 	Added vm_page_fictitious_addr.
 * 	[91/04/10            rpd]
 * 
 * 	Restored vm_page_laundry_count.
 * 	[91/04/07            rpd]
 * 
 * 	Changed vm_page_release to use thread_wakeup_one.
 * 	[91/04/05            rpd]
 * 	Added vm_page_grab_fictitious, etc.
 * 	[91/03/29            rpd]
 * 	Added vm_page_bootstrap, pmap_startup, pmap_steal_memory.
 * 	[91/03/25            rpd]
 * 
 * Revision 2.12  91/05/14  17:51:19  mrt
 * 	Correcting copyright
 * 
 * Revision 2.11  91/03/16  15:07:02  rpd
 * 	Reverted to the previous version of vm_page_deactivate,
 * 	which doesn't look at the busy bit.  Changed vm_page_alloc
 * 	to not deactivate busy pages.
 * 	[91/03/11            rpd]
 * 
 * 	Fixed simple-locking typo.
 * 	[91/03/09            rpd]
 * 	Added continuation argument to vm_page_wait.
 * 	[91/02/05            rpd]
 * 
 * Revision 2.10  91/02/05  18:00:27  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  16:34:44  mrt]
 * 
 * Revision 2.9  91/01/08  16:46:06  rpd
 * 	Changed to singly-linked VP bucket chains.
 * 	[91/01/03            rpd]
 * 
 * 	Removed count field from VP buckets.
 * 	Added vm_page_info.
 * 	[91/01/02            rpd]
 * 	Added vm_page_grab, vm_page_release.
 * 	Changed vm_wait/VM_WAIT to vm_page_wait/VM_PAGE_WAIT.
 * 	[90/12/09  17:41:15  rpd]
 * 
 * Revision 2.8  90/11/05  14:35:12  rpd
 * 	Changed vm_page_deactivate to remove busy pages from the page queues.
 * 	Now it requires that the page's object be locked.
 * 	[90/11/04            rpd]
 * 
 * Revision 2.7  90/10/25  14:50:50  rwd
 * 	Made vm_page_alloc_deactivate_behind TRUE.
 * 	[90/10/24            rwd]
 * 
 * 	Removed the max_mapping field of pages.
 * 	[90/10/22            rpd]
 * 
 * Revision 2.6  90/10/12  13:07:03  rpd
 * 	Initialize vm_page_template's software reference bit.
 * 	In vm_page_deactivate, clear the software reference bit
 * 	in addition to using pmap_clear_reference.
 * 	[90/10/08            rpd]
 * 
 * Revision 2.5  90/08/27  22:16:11  dbg
 * 	Fixed vm_page_free, vm_page_wire, vm_page_unwire
 * 	to only modify vm_page_wire_count for real pages.
 * 	[90/08/23            rpd]
 * 
 * Revision 2.4  90/02/22  20:06:53  dbg
 * 	Fix vm_page_deactivate to work for pages that are wired or
 * 	already inactive.
 * 	[90/02/09            dbg]
 * 		PAGE_WAKEUP --> PAGE_WAKEUP_DONE in vm_page_free() to reflect
 * 		the fact that it clears the busy flag.  Remove PAGE_WAKEUP from
 * 		vm_page_unwire; callers are responsible for this, and it didn't
 * 		work right if the page was wired more than once.
 * 		[89/12/13            dlb]
 * 
 * Revision 2.3  90/01/11  11:48:34  dbg
 * 	Removed all spl protection from VM system.
 * 	Removed vm_page_free_synchronized.
 * 	[90/01/03            dbg]
 * 
 * 	Added changes from mainline:
 * 
 * 		Retract special preemption technology for pageout daemon.
 * 		[89/10/10            mwyoung]
 * 
 * 		Add documentation of global variables.
 * 		Declare vm_page_bucket_t for VP table; add count and
 * 		lock fields.
 * 		[89/04/29            mwyoung]
 * 
 * 		Separate "private" from "fictitious" page attributes.
 * 		[89/04/22            mwyoung]
 * 
 * 		Made the deactivate-behind optimization conditional on
 * 		vm_page_alloc_deactivate_behind, which is FALSE for now.
 * 		[89/08/31  19:32:59  rpd]
 * 
 * 		Increased zdata_size to allow for more zones.
 * 		[89/07/31  17:13:06  jsb]
 * 
 * 		Changed from 8 to 15 the threshold that triggers invocation of
 * 		Debugger() in vm_page_alloc().  On the M500 with few buffers
 * 		was causing trouble. [af]
 * 
 * Revision 2.2  89/09/08  11:29:00  dbg
 * 	Fixed vm_page_free to decrement vm_page_laundry_count
 * 	only if the freed page was in the laundry.  Also
 * 	made vm_page_free fix vm_page_wire_count when freeing
 * 	a wired page.
 * 
 * 	Revision 2.16  89/06/12  14:53:18  jsb
 * 		Picked up bug fix (missing splimp) from Sequent via dlb.
 * 		[89/06/12  14:38:34  jsb]
 * 
 * 	Revision 2.15  89/06/02  11:38:00  rvb
 * 		Changed from 8 to 15 the threshold that triggers invocation of
 * 		Debugger() in vm_page_alloc().  On the M500 with few buffers
 * 		was causing trouble. [af]
 * 
 * Revision 2.14  89/04/18  21:29:17  mwyoung
 * 	Recent history:
 * 		Add vm_page_fictitious_zone.
 * 		Handle absent pages in vm_page_free().
 * 		Eliminate use of owner and clean fields to vm_page_t.
 * 	History condensation:
 * 	 	Reorganize vm_page_startup to avoid bad physical addresses.
 * 		Use a template for initialization [mwyoung].
 * 		Provide separate vm_page_init() function for outside use [mwyoung].
 * 		Split up page system lock [dbg].
 * 		Initial external memory management integration [bolosky, mwyoung].
 * 		Plenty of bug fixes [dbg, avie, mwyoung].
 * 		Converted to active/inactive/free list queues [avie].
 * 		Created [avie].
 * 	[89/04/18            mwyoung]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
 *	File:	vm/vm_page.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Resident memory management module.
 */

#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <kern/counters.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/xpr.h>
#include <vm/pmap.h>
#include <vm/vm_init.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_kern.h>			/* kernel_memory_allocate() */
#include <kern/misc_protos.h>
#include <zone_debug.h>
#include <vm/cpm.h>

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

/*
 *	These variables record the values returned by vm_page_bootstrap,
 *	for debugging purposes.  The implementation of pmap_steal_memory
 *	and pmap_startup here also uses them internally.
 */

vm_offset_t virtual_space_start;
vm_offset_t virtual_space_end;
int	vm_page_pages;

/*
 *	The vm_page_lookup() routine, which provides for fast
 *	(virtual memory object, offset) to page lookup, employs
 *	the following hash table.  The vm_page_{insert,remove}
 *	routines install and remove associations in the table.
 *	[This table is often called the virtual-to-physical,
 *	or VP, table.]
 */
typedef struct {
	decl_mutex_data(,lock)
	vm_page_t	pages;
#if	MACH_PAGE_HASH_STATS
	int		cur_count;		/* current count */
	int		hi_count;		/* high water mark */
#endif /* MACH_PAGE_HASH_STATS */
} vm_page_bucket_t;

vm_page_bucket_t *vm_page_buckets;		/* Array of buckets */
unsigned int	vm_page_bucket_count = 0;	/* How big is array? */
unsigned int	vm_page_hash_mask;		/* Mask for hash function */
unsigned int	vm_page_hash_shift;		/* Shift for hash function */

#if	MACH_PAGE_HASH_STATS
/* This routine is only for debug.  It is intended to be called by
 * hand by a developer using a kernel debugger.  This routine prints
 * out vm_page_hash table statistics to the kernel debug console.
 */
void
hash_debug(void)
{
	int	i;
	int	numbuckets = 0;
	int	highsum = 0;
	int	maxdepth = 0;

	for (i = 0; i < vm_page_bucket_count; i++) {
		if (vm_page_buckets[i].hi_count) {
			numbuckets++;
			highsum += vm_page_buckets[i].hi_count;
			if (vm_page_buckets[i].hi_count > maxdepth)
				maxdepth = vm_page_buckets[i].hi_count;
		}
	}
	printf("Total number of buckets: %d\n", vm_page_bucket_count);
	printf("Number used buckets:     %d = %d%%\n",
		numbuckets, 100*numbuckets/vm_page_bucket_count);
	printf("Number unused buckets:   %d = %d%%\n",
		vm_page_bucket_count - numbuckets,
		100*(vm_page_bucket_count-numbuckets)/vm_page_bucket_count);
	printf("Sum of bucket max depth: %d\n", highsum);
	printf("Average bucket depth:    %d.%2d\n",
		highsum/vm_page_bucket_count,
		highsum%vm_page_bucket_count);
	printf("Maximum bucket depth:    %d\n", maxdepth);
}
#endif /* MACH_PAGE_HASH_STATS */

/*
 *	The virtual page size is currently implemented as a runtime
 *	variable, but is constant once initialized using vm_set_page_size.
 *	This initialization must be done in the machine-dependent
 *	bootstrap sequence, before calling other machine-independent
 *	initializations.
 *
 *	All references to the virtual page size outside this
 *	module must use the PAGE_SIZE, PAGE_MASK and PAGE_SHIFT
 *	constants.
 */
#ifndef PAGE_SIZE_FIXED
vm_size_t	page_size  = 4096;
vm_size_t	page_mask  = 4095;
int		page_shift = 12;
#endif /* PAGE_SIZE_FIXED */

/*
 *	Resident page structures are initialized from
 *	a template (see vm_page_alloc).
 *
 *	When adding a new field to the virtual memory
 *	object structure, be sure to add initialization
 *	(see vm_page_bootstrap).
 */
struct vm_page	vm_page_template;

/*
 *	Resident pages that represent real memory
 *	are allocated from a free list.
 */
vm_page_t	vm_page_queue_free;
vm_page_t       vm_page_queue_fictitious;
decl_mutex_data(,vm_page_queue_free_lock)
unsigned int	vm_page_free_wanted;
int		vm_page_free_count;
int		vm_page_fictitious_count;

unsigned int	vm_page_free_count_minimum;	/* debugging */

/*
 *	Occasionally, the virtual memory system uses
 *	resident page structures that do not refer to
 *	real pages, for example to leave a page with
 *	important state information in the VP table.
 *
 *	These page structures are allocated the way
 *	most other kernel structures are.
 */
zone_t	vm_page_zone;
decl_mutex_data(,vm_page_alloc_lock)

/*
 *	Fictitious pages don't have a physical address,
 *	but we must initialize phys_addr to something.
 *	For debugging, this should be a strange value
 *	that the pmap module can recognize in assertions.
 */
vm_offset_t vm_page_fictitious_addr = (vm_offset_t) -1;

/*
 *	Resident page structures are also chained on
 *	queues that are used by the page replacement
 *	system (pageout daemon).  These queues are
 *	defined here, but are shared by the pageout
 *	module.
 */
queue_head_t	vm_page_queue_active;
queue_head_t	vm_page_queue_inactive;
decl_mutex_data(,vm_page_queue_lock)
int		vm_page_active_count;
int		vm_page_inactive_count;
int		vm_page_wire_count;
int		vm_page_gobble_count = 0;
int		vm_page_wire_count_warning = 0;
int		vm_page_gobble_count_warning = 0;

/* the following fields are protected by the vm_page_queue_lock */
queue_head_t	vm_page_queue_limbo;
int	vm_page_limbo_count = 0;		/* total pages in limbo */
int	vm_page_limbo_real_count = 0;		/* real pages in limbo */
int	vm_page_pin_count = 0;			/* number of pinned pages */

decl_simple_lock_data(,vm_page_preppin_lock)

/*
 *	Several page replacement parameters are also
 *	shared with this module, so that page allocation
 *	(done here in vm_page_alloc) can trigger the
 *	pageout daemon.
 */
int	vm_page_free_target = 0;
int	vm_page_free_min = 0;
int	vm_page_inactive_target = 0;
int	vm_page_free_reserved = 0;
int	vm_page_laundry_count = 0;

/*
 *	The VM system has a couple of heuristics for deciding
 *	that pages are "uninteresting" and should be placed
 *	on the inactive queue as likely candidates for replacement.
 *	These variables let the heuristics be controlled at run-time
 *	to make experimentation easier.
 */

boolean_t vm_page_deactivate_hint = TRUE;

/* 
 * Definition for automatic physical memory reservation.  Declared in
 * vm_page.h, defined here.  See vm_page.h for details.  There's an
 * extra zero entry to allow the code to compile if there are no requests
 * for physical memory allocation; pmem_reserve_ctl_size is decremented
 * by one to compensate.
 */
#include <flipc.h>
#if FLIPC
#include <flipc/flipc_usermsg.h>
#endif

struct pmem_reserve pmem_reserve_ctl_array[] = {
#if FLIPC
    { &flipc_cb_length, (vm_offset_t *) &flipc_cb_base },
#endif
    { 0, (vm_offset_t *) 0 }
};

struct pmem_reserve *pmem_reserve_ctl = &pmem_reserve_ctl_array[0];

int pmem_reserve_ctl_size =
  (sizeof(pmem_reserve_ctl_array) / sizeof(struct pmem_reserve)) - 1;

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from page_size.
 */
void
vm_set_page_size(void)
{
#ifndef PAGE_SIZE_FIXED
	page_mask = page_size - 1;

	if ((page_mask & page_size) != 0)
		panic("vm_set_page_size: page size not a power of two");

	for (page_shift = 0; ; page_shift++)
		if ((1 << page_shift) == page_size)
			break;
#endif /* PAGE_SIZE_FIXED */
}

/*
 *	vm_page_bootstrap:
 *
 *	Initializes the resident memory module.
 *
 *	Allocates memory for the page cells, and
 *	for the object/offset-to-page hash table headers.
 *	Each page cell is initialized and placed on the free list.
 *	Returns the range of available kernel virtual memory.
 */

void
vm_page_bootstrap(
	vm_offset_t		*startp,
	vm_offset_t		*endp)
{
	register vm_page_t	m;
	int			i;
	unsigned int		log1;
	unsigned int		log2;
	unsigned int		size;

	/*
	 *	Initialize the vm_page template.
	 */

	m = &vm_page_template;
	m->object = VM_OBJECT_NULL;	/* reset later */
	m->offset = 0;			/* reset later */
	m->wire_count = 0;

	m->inactive = FALSE;
	m->active = FALSE;
	m->laundry = FALSE;
	m->free = FALSE;
	m->reference = FALSE;
	m->pageout = FALSE;

	m->busy = TRUE;
	m->wanted = FALSE;
	m->tabled = FALSE;
	m->fictitious = FALSE;
	m->private = FALSE;
	m->absent = FALSE;
	m->error = FALSE;
	m->dirty = FALSE;
	m->cleaning = FALSE;
	m->precious = FALSE;
	m->clustered = FALSE;
	m->lock_supplied = FALSE;
	m->unusual = FALSE;
	m->restart = FALSE;
	m->limbo = FALSE;

	m->phys_addr = 0;		/* reset later */

	m->page_lock = VM_PROT_NONE;
	m->unlock_request = VM_PROT_NONE;
	m->page_error = KERN_SUCCESS;

	/*
	 *	Initialize the page queues.
	 */

	mutex_init(&vm_page_queue_free_lock, ETAP_VM_PAGEQ_FREE);
	mutex_init(&vm_page_queue_lock, ETAP_VM_PAGEQ);
	simple_lock_init(&vm_page_preppin_lock, ETAP_VM_PREPPIN);

	vm_page_queue_free = VM_PAGE_NULL;
	vm_page_queue_fictitious = VM_PAGE_NULL;
	queue_init(&vm_page_queue_active);
	queue_init(&vm_page_queue_inactive);
	queue_init(&vm_page_queue_limbo);

	vm_page_free_wanted = 0;

	/*
	 *	Steal memory for the map and zone subsystems.
	 */

	vm_map_steal_memory();
	zone_steal_memory();

	/*
	 *	Allocate (and initialize) the virtual-to-physical
	 *	table hash buckets.
	 *
	 *	The number of buckets should be a power of two to
	 *	get a good hash function.  The following computation
	 *	chooses the first power of two that is greater
	 *	than the number of physical pages in the system.
	 */

	if (vm_page_bucket_count == 0) {
		unsigned int npages = pmap_free_pages();

		vm_page_bucket_count = 1;
		while (vm_page_bucket_count < npages)
			vm_page_bucket_count <<= 1;
	}

	vm_page_hash_mask = vm_page_bucket_count - 1;

	/*
	 *	Calculate object shift value for hashing algorithm:
	 *		O = log2(sizeof(struct vm_object))
	 *		B = log2(vm_page_bucket_count)
	 *	        hash shifts the object left by
	 *		B/2 - O
	 */
	size = vm_page_bucket_count;
	for (log1 = 0; size > 1; log1++) 
		size /= 2;
	size = sizeof(struct vm_object);
	for (log2 = 0; size > 1; log2++) 
		size /= 2;
	vm_page_hash_shift = log1/2 - log2 + 1;

	if (vm_page_hash_mask & vm_page_bucket_count)
		printf("vm_page_bootstrap: WARNING -- strange page hash\n");

	vm_page_buckets = (vm_page_bucket_t *)
		pmap_steal_memory(vm_page_bucket_count *
				  sizeof(vm_page_bucket_t));

	for (i = 0; i < vm_page_bucket_count; i++) {
		register vm_page_bucket_t *bucket = &vm_page_buckets[i];

		bucket->pages = VM_PAGE_NULL;
		mutex_init(&bucket->lock, ETAP_VM_BUCKET);
#if     MACH_PAGE_HASH_STATS
		bucket->cur_count = 0;
		bucket->hi_count = 0;
#endif /* MACH_PAGE_HASH_STATS */
	}

	/*
	 *	Machine-dependent code allocates the resident page table.
	 *	It uses vm_page_init to initialize the page frames.
	 *	The code also returns to us the virtual space available
	 *	to the kernel.  We don't trust the pmap module
	 *	to get the alignment right.
	 */

	pmap_startup(&virtual_space_start, &virtual_space_end);
	virtual_space_start = round_page(virtual_space_start);
	virtual_space_end = trunc_page(virtual_space_end);

	*startp = virtual_space_start;
	*endp = virtual_space_end;

	/*
	 *	Compute the initial "wire" count.
	 *	Up until now, the pages which have been set aside are not under 
	 *	the VM system's control, so although they aren't explicitly
	 *	wired, they nonetheless can't be moved. At this moment,
	 *	all VM managed pages are "free", courtesy of pmap_startup.
	 */
	vm_page_wire_count = atop(mem_size) - vm_page_free_count;	/* initial value */

	printf("vm_page_bootstrap: %d free pages\n", vm_page_free_count);
	vm_page_free_count_minimum = vm_page_free_count;
}

#ifndef	MACHINE_PAGES
/*
 *	We implement pmap_steal_memory and pmap_startup with the help
 *	of two simpler functions, pmap_virtual_space and pmap_next_page.
 */

vm_offset_t
pmap_steal_memory(
	vm_size_t size)
{
	vm_offset_t addr, vaddr, paddr;

	/*
	 *	We round the size to a round multiple.
	 */

	size = (size + sizeof (void *) - 1) &~ (sizeof (void *) - 1);

	/*
	 *	If this is the first call to pmap_steal_memory,
	 *	we have to initialize ourself.
	 */

	if (virtual_space_start == virtual_space_end) {
		pmap_virtual_space(&virtual_space_start, &virtual_space_end);

		/*
		 *	The initial values must be aligned properly, and
		 *	we don't trust the pmap module to do it right.
		 */

		virtual_space_start = round_page(virtual_space_start);
		virtual_space_end = trunc_page(virtual_space_end);
	}

	/*
	 *	Allocate virtual memory for this request.
	 */

	addr = virtual_space_start;
	virtual_space_start += size;

	/*
	 *	Allocate and map physical pages to back new virtual pages.
	 */

	for (vaddr = round_page(addr);
	     vaddr < addr + size;
	     vaddr += PAGE_SIZE) {
		if (!pmap_next_page(&paddr))
			panic("pmap_steal_memory");

		/*
		 *	XXX Logically, these mappings should be wired,
		 *	but some pmap modules barf if they are.
		 */

		pmap_enter(kernel_pmap, vaddr, paddr,
			   VM_PROT_READ|VM_PROT_WRITE, FALSE);
		/*
		 * Account for newly stolen memory
		 */
		vm_page_wire_count++;

	}

	return addr;
}

void
pmap_startup(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	unsigned int i, npages, pages_initialized;
	vm_page_t pages;
	vm_offset_t paddr;

	/*
	 *	We calculate how many page frames we will have
	 *	and then allocate the page structures in one chunk.
	 */

	npages = ((PAGE_SIZE * pmap_free_pages() +
		   (round_page(virtual_space_start) - virtual_space_start)) /
		  (PAGE_SIZE + sizeof *pages));

	pages = (vm_page_t) pmap_steal_memory(npages * sizeof *pages);

	/*
	 *	Initialize the page frames.
	 */

	for (i = 0, pages_initialized = 0; i < npages; i++) {
		if (!pmap_next_page(&paddr))
			break;

		vm_page_init(&pages[i], paddr);
		vm_page_pages++;
		pages_initialized++;
	}

	/*
	 * Release pages in reverse order so that physical pages
	 * initially get allocated in ascending addresses. This keeps
	 * the devices (which must address physical memory) happy if
	 * they require several consecutive pages.
	 */

	for (i = pages_initialized; i > 0; i--) {
		vm_page_release(&pages[i - 1]);
	}

	/*
	 *	We have to re-align virtual_space_start,
	 *	because pmap_steal_memory has been using it.
	 */

	virtual_space_start = round_page(virtual_space_start);

	*startp = virtual_space_start;
	*endp = virtual_space_end;
}
#endif	/* MACHINE_PAGES */

/*
 *	Routine:	vm_page_module_init
 *	Purpose:
 *		Second initialization pass, to be done after
 *		the basic VM system is ready.
 */
void
vm_page_module_init(void)
{
	vm_page_zone = zinit((vm_size_t) sizeof(struct vm_page),
			     0, PAGE_SIZE, "vm pages");
#if	ZONE_DEBUG
	zone_debug_disable(vm_page_zone);
#endif	/* ZONE_DEBUG */

	zone_change(vm_page_zone, Z_EXPAND, FALSE);
	zone_change(vm_page_zone, Z_EXHAUST, TRUE);
	zone_change(vm_page_zone, Z_FOREIGN, TRUE);

        /*
         * Adjust zone statistics to account for the real pages allocated
         * in vm_page_create(). [Q: is this really what we want?]
         */
        vm_page_zone->count += vm_page_pages;
        vm_page_zone->cur_size += vm_page_pages * vm_page_zone->elem_size;

	mutex_init(&vm_page_alloc_lock, ETAP_VM_PAGE_ALLOC);
}

/*
 *	Routine:	vm_page_create
 *	Purpose:
 *		After the VM system is up, machine-dependent code
 *		may stumble across more physical memory.  For example,
 *		memory that it was reserving for a frame buffer.
 *		vm_page_create turns this memory into available pages.
 */

void
vm_page_create(
	vm_offset_t start,
	vm_offset_t end)
{
	vm_offset_t paddr;
	vm_page_t m;

	for (paddr = round_page(start);
	     paddr < trunc_page(end);
	     paddr += PAGE_SIZE) {
		while ((m = (vm_page_t) vm_page_grab_fictitious())
			== VM_PAGE_NULL)
			vm_page_more_fictitious();

		vm_page_init(m, paddr);
		vm_page_pages++;
		vm_page_release(m);
	}
}

/*
 *	vm_page_hash:
 *
 *	Distributes the object/offset key pair among hash buckets.
 *
 *	NOTE:	To get a good hash function, the bucket count should
 *		be a power of two.
 */
#define vm_page_hash(object, offset) (\
	( ((natural_t)(vm_offset_t)object<<vm_page_hash_shift) + (natural_t)atop(offset))\
	 & vm_page_hash_mask)

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the object/object-page
 *	table and object list.
 *
 *	The object must be locked.
 */

void
vm_page_insert(
	register vm_page_t	mem,
	register vm_object_t	object,
	register vm_offset_t	offset)
{
	register vm_page_bucket_t *bucket;

        XPR(XPR_VM_PAGE,
                "vm_page_insert, object 0x%X offset 0x%X page 0x%X\n",
                (integer_t)object, (integer_t)offset, (integer_t)mem, 0,0);

	VM_PAGE_CHECK(mem);

	if (mem->tabled)
		panic("vm_page_insert");

	assert(!object->internal || offset < object->size);

	/* only insert "pageout" pages into "pageout" objects,
	 * and normal pages into normal objects */
	assert(object->pageout == mem->pageout);

	/*
	 *	Record the object/offset pair in this page
	 */

	mem->object = object;
	mem->offset = offset;

	/*
	 *	Insert it into the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];
	mutex_lock(&bucket->lock);
	mem->next = bucket->pages;
	bucket->pages = mem;
#if     MACH_PAGE_HASH_STATS
	if (++bucket->cur_count > bucket->hi_count)
		bucket->hi_count = bucket->cur_count;
#endif /* MACH_PAGE_HASH_STATS */
	mutex_unlock(&bucket->lock);

	/*
	 *	Now link into the object's list of backed pages.
	 */

	queue_enter(&object->memq, mem, vm_page_t, listq);
	mem->tabled = TRUE;

	/*
	 *	Show that the object has one more resident page.
	 */

	object->resident_page_count++;
}

/*
 *	vm_page_replace:
 *
 *	Exactly like vm_page_insert, except that we first
 *	remove any existing page at the given offset in object.
 *
 *	The object and page queues must be locked.
 */

void
vm_page_replace(
	register vm_page_t	mem,
	register vm_object_t	object,
	register vm_offset_t	offset)
{
	register vm_page_bucket_t *bucket;

	VM_PAGE_CHECK(mem);

	if (mem->tabled)
		panic("vm_page_replace");

	/*
	 *	Record the object/offset pair in this page
	 */

	mem->object = object;
	mem->offset = offset;

	/*
	 *	Insert it into the object_object/offset hash table,
	 *	replacing any page that might have been there.
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];
	mutex_lock(&bucket->lock);
	if (bucket->pages) {
		vm_page_t *mp = &bucket->pages;
		register vm_page_t m = *mp;
		do {
			if (m->object == object && m->offset == offset) {
				/*
				 * Remove page from bucket and from object,
				 * and return it to the free list.
				 */
				*mp = m->next;
				queue_remove(&object->memq, m, vm_page_t,
					     listq);
				m->tabled = FALSE;
				object->resident_page_count--;

				/*
				 * Return page to the free list.
				 * Note the page is not tabled now, so this
				 * won't self-deadlock on the bucket lock.
				 */

				vm_page_free(m);
				break;
			}
			mp = &m->next;
		} while (m = *mp);
		mem->next = bucket->pages;
	} else {
		mem->next = VM_PAGE_NULL;
	}
	bucket->pages = mem;
	mutex_unlock(&bucket->lock);

	/*
	 *	Now link into the object's list of backed pages.
	 */

	queue_enter(&object->memq, mem, vm_page_t, listq);
	mem->tabled = TRUE;

	/*
	 *	And show that the object has one more resident
	 *	page.
	 */

	object->resident_page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 *
 *	Removes the given mem entry from the object/offset-page
 *	table and the object page list.
 *
 *	The object and page must be locked.
 */

void
vm_page_remove(
	register vm_page_t	mem)
{
	register vm_page_bucket_t	*bucket;
	register vm_page_t	this;

        XPR(XPR_VM_PAGE,
                "vm_page_remove, object 0x%X offset 0x%X page 0x%X\n",
                (integer_t)mem->object, (integer_t)mem->offset, 
		(integer_t)mem, 0,0);

	assert(mem->tabled);
	assert(!mem->cleaning);
	VM_PAGE_CHECK(mem);

	/*
	 *	Remove from the object_object/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(mem->object, mem->offset)];
	mutex_lock(&bucket->lock);
	if ((this = bucket->pages) == mem) {
		/* optimize for common case */

		bucket->pages = mem->next;
	} else {
		register vm_page_t	*prev;

		for (prev = &this->next;
		     (this = *prev) != mem;
		     prev = &this->next)
			continue;
		*prev = this->next;
	}
#if     MACH_PAGE_HASH_STATS
	bucket->cur_count--;
#endif /* MACH_PAGE_HASH_STATS */
	mutex_unlock(&bucket->lock);

	/*
	 *	Now remove from the object's list of backed pages.
	 */

	queue_remove(&mem->object->memq, mem, vm_page_t, listq);

	/*
	 *	And show that the object has one fewer resident
	 *	page.
	 */

	mem->object->resident_page_count--;

	mem->tabled = FALSE;
	mem->object = VM_OBJECT_NULL;
	mem->offset = 0;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the object/offset
 *	pair specified; if none is found, VM_PAGE_NULL is returned.
 *
 *	The object must be locked.  No side effects.
 */

vm_page_t
vm_page_lookup(
	register vm_object_t	object,
	register vm_offset_t	offset)
{
	register vm_page_t	mem;
	register vm_page_bucket_t *bucket;

	/*
	 *	Search the hash table for this object/offset pair
	 */

	bucket = &vm_page_buckets[vm_page_hash(object, offset)];

	mutex_lock(&bucket->lock);
	for (mem = bucket->pages; mem != VM_PAGE_NULL; mem = mem->next) {
		VM_PAGE_CHECK(mem);
		if ((mem->object == object) && (mem->offset == offset))
			break;
	}
	mutex_unlock(&bucket->lock);
	return(mem);
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current object to the specified target object/offset.
 *
 *	The object must be locked.
 */
void
vm_page_rename(
	register vm_page_t	mem,
	register vm_object_t	new_object,
	vm_offset_t		new_offset)
{
	assert(mem->object != new_object);
	/*
	 *	Changes to mem->object require the page lock because
	 *	the pageout daemon uses that lock to get the object.
	 */

        XPR(XPR_VM_PAGE,
                "vm_page_rename, new object 0x%X, offset 0x%X page 0x%X\n",
                (integer_t)new_object, (integer_t)new_offset, 
		(integer_t)mem, 0,0);

	vm_page_lock_queues();
    	vm_page_remove(mem);
	vm_page_insert(mem, new_object, new_offset);
	vm_page_unlock_queues();
}

/*
 *	vm_page_init:
 *
 *	Initialize the fields in a new page.
 *	This takes a structure with random values and initializes it
 *	so that it can be given to vm_page_release or vm_page_insert.
 */
void
vm_page_init(
	vm_page_t	mem,
	vm_offset_t	phys_addr)
{
	*mem = vm_page_template;
	mem->phys_addr = phys_addr;
}

/*
 *	vm_page_grab_fictitious:
 *
 *	Remove a fictitious page from the free list.
 *	Returns VM_PAGE_NULL if there are no free pages.
 */
int	c_vm_page_grab_fictitious = 0;
int	c_vm_page_release_fictitious = 0;
int	c_vm_page_more_fictitious = 0;

vm_page_t
vm_page_grab_fictitious(void)
{
	register vm_page_t m;

	m = (vm_page_t)zget(vm_page_zone);
	if (m) {
		m->free = FALSE;
#if	MACH_ASSERT || ZONE_DEBUG
		vm_page_init(m, vm_page_fictitious_addr);
		m->fictitious = TRUE;
#endif	/* MACH_ASSERT || ZONE_DEBUG */
	}

	c_vm_page_grab_fictitious++;
	return m;
}

/*
 *	vm_page_release_fictitious:
 *
 *	Release a fictitious page to the free list.
 */

void
vm_page_release_fictitious(
	register vm_page_t m)
{
	assert(!m->free);
	assert(m->busy);
	assert(m->fictitious);
	assert(m->phys_addr == vm_page_fictitious_addr);

	c_vm_page_release_fictitious++;

	if (m->free)
		panic("vm_page_release_fictitious");
	m->free = TRUE;
	zfree(vm_page_zone, (vm_offset_t)m);
}

/*
 *	vm_page_more_fictitious:
 *
 *	Add more fictitious pages to the free list.
 *	Allowed to block. This routine is way intimate
 *	with the zones code, for several reasons:
 *	1. we need to carve some page structures out of physical
 *	   memory before zones work, so they _cannot_ come from
 *	   the zone_map.
 *	2. the zone needs to be collectable in order to prevent
 *	   growth without bound. These structures are used by
 *	   the device pager (by the hundreds and thousands), as
 *	   private pages for pageout, and as blocking pages for
 *	   pagein. Temporary bursts in demand should not result in
 *	   permanent allocation of a resource.
 *	3. To smooth allocation humps, we allocate single pages
 *	   with kernel_memory_allocate(), and cram them into the
 *	   zone. This also allows us to initialize the vm_page_t's
 *	   on the way into the zone, so that zget() always returns
 *	   an initialized structure. The zone free element pointer
 *	   and the free page pointer are both the first item in the
 *	   vm_page_t.
 *	4. By having the pages in the zone pre-initialized, we need
 *	   not keep 2 levels of lists. The garbage collector simply
 *	   scans our list, and reduces physical memory usage as it
 *	   sees fit.
 */

void vm_page_more_fictitious(void)
{
	extern vm_map_t zone_map;
	register vm_page_t m;
	vm_offset_t addr;
	kern_return_t retval;
	int i;

	c_vm_page_more_fictitious++;

	/* this may free up some fictitious pages */
	cleanup_limbo_queue();

	/*
	 * Allocate a single page from the zone_map. Do not wait if no physical
	 * pages are immediately available, and do not zero the space. We need
	 * our own blocking lock here to prevent having multiple,
	 * simultaneous requests from piling up on the zone_map lock. Exactly
	 * one (of our) threads should be potentially waiting on the map lock.
	 * If winner is not vm-privileged, then the page allocation will fail,
	 * and it will temporarily block here in the vm_page_wait().
	 */
	mutex_lock(&vm_page_alloc_lock);
	/*
	 * If another thread allocated space, just bail out now.
	 */
	if (zone_free_count(vm_page_zone) > 5) {
		/*
		 * The number "5" is a small number that is larger than the
		 * number of fictitious pages that any single caller will
		 * attempt to allocate. Otherwise, a thread will attempt to
		 * acquire a fictitious page (vm_page_grab_fictitious), fail,
		 * release all of the resources and locks already acquired,
		 * and then call this routine. This routine finds the pages
		 * that the caller released, so fails to allocate new space.
		 * The process repeats infinitely. The largest known number
		 * of fictitious pages required in this manner is 2. 5 is
		 * simply a somewhat larger number.
		 */
		mutex_unlock(&vm_page_alloc_lock);
		return;
	}

	if ((retval = kernel_memory_allocate(zone_map,
			&addr, PAGE_SIZE, VM_PROT_ALL,
			KMA_KOBJECT|KMA_NOPAGEWAIT)) != KERN_SUCCESS) { 
		/*
		 * No page was available. Tell the pageout daemon, drop the
		 * lock to give another thread a chance at it, and
		 * wait for the pageout daemon to make progress.
		 */
		mutex_unlock(&vm_page_alloc_lock);
		vm_page_wait();
		return;
	}
	/*
	 * Initialize as many vm_page_t's as will fit on this page. This
	 * depends on the zone code disturbing ONLY the first item of
	 * each zone element.
	 */
	m = (vm_page_t)addr;
	for (i = PAGE_SIZE/sizeof(struct vm_page); i > 0; i--) {
		vm_page_init(m, vm_page_fictitious_addr);
		m->fictitious = TRUE;
		m++;
	}
	zcram(vm_page_zone, addr, PAGE_SIZE);
	mutex_unlock(&vm_page_alloc_lock);
}

/*
 *	vm_page_convert:
 *
 *	Attempt to convert a fictitious page into a real page.
 */

boolean_t
vm_page_convert(
	register vm_page_t m)
{
	register vm_page_t real_m;

	assert(m->busy);
	assert(m->fictitious);
	assert(!m->dirty);

	real_m = vm_page_grab();
	if (real_m == VM_PAGE_NULL)
		return FALSE;

	m->phys_addr = real_m->phys_addr;
	m->fictitious = FALSE;

	vm_page_lock_queues();
	if (m->active)
		vm_page_active_count++;
	else if (m->inactive)
		vm_page_inactive_count++;
	vm_page_unlock_queues();

	real_m->phys_addr = vm_page_fictitious_addr;
	real_m->fictitious = TRUE;

	vm_page_release_fictitious(real_m);
	return TRUE;
}

/*
 *	vm_pool_low():
 *
 *	Return true if it is not likely that a non-vm_privileged thread
 *	can get memory without blocking.  Advisory only, since the
 *	situation may change under us.
 */
int
vm_pool_low(void)
{
	/* No locking, at worst we will fib. */
	return( vm_page_free_count < vm_page_free_reserved );
}

/*
 *	vm_page_grab:
 *
 *	Remove a page from the free list.
 *	Returns VM_PAGE_NULL if the free list is too small.
 */

unsigned long	vm_page_grab_count = 0;	/* measure demand */

vm_page_t
vm_page_grab(void)
{
	register vm_page_t	mem;

	mutex_lock(&vm_page_queue_free_lock);
	vm_page_grab_count++;

	/*
	 *	Optionally produce warnings if the wire or gobble
	 *	counts exceed some threshold.
	 */
	if (vm_page_wire_count_warning > 0
	    && vm_page_wire_count >= vm_page_wire_count_warning) {
		printf("mk: vm_page_grab(): high wired page count of %d\n",
			vm_page_wire_count);
		assert(vm_page_wire_count < vm_page_wire_count_warning);
	}
	if (vm_page_gobble_count_warning > 0
	    && vm_page_gobble_count >= vm_page_gobble_count_warning) {
		printf("mk: vm_page_grab(): high gobbled page count of %d\n",
			vm_page_gobble_count);
		assert(vm_page_gobble_count < vm_page_gobble_count_warning);
	}

	/*
	 *	Only let privileged threads (involved in pageout)
	 *	dip into the reserved pool.
	 */

	if ((vm_page_free_count < vm_page_free_reserved) &&
	    !current_thread()->vm_privilege) {
		mutex_unlock(&vm_page_queue_free_lock);
		return VM_PAGE_NULL;
	}

	while (vm_page_queue_free == VM_PAGE_NULL) {
		printf("vm_page_grab: no free pages, trouble expected...\n");
		mutex_unlock(&vm_page_queue_free_lock);
		VM_PAGE_WAIT();
		mutex_lock(&vm_page_queue_free_lock);
	}

	if (--vm_page_free_count < vm_page_free_count_minimum)
		vm_page_free_count_minimum = vm_page_free_count;
	mem = vm_page_queue_free;
	vm_page_queue_free = (vm_page_t) mem->pageq.next;
	mem->free = FALSE;
	mutex_unlock(&vm_page_queue_free_lock);

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if ((vm_page_free_count < vm_page_free_min) ||
	    ((vm_page_free_count < vm_page_free_target) &&
	     (vm_page_inactive_count < vm_page_inactive_target)))
		thread_wakeup((event_t) &vm_page_free_wanted);

	return mem;
}

/*
 *	vm_page_release:
 *
 *	Return a page to the free list.
 */

void
vm_page_release(
	register vm_page_t	mem)
{
	assert(!mem->private && !mem->fictitious);

	mutex_lock(&vm_page_queue_free_lock);
	if (mem->free)
		panic("vm_page_release");
	mem->free = TRUE;
	mem->pageq.next = (queue_entry_t) vm_page_queue_free;
	vm_page_queue_free = mem;
	vm_page_free_count++;

	/*
	 *	Check if we should wake up someone waiting for page.
	 *	But don't bother waking them unless they can allocate.
	 *
	 *	We wakeup only one thread, to prevent starvation.
	 *	Because the scheduling system handles wait queues FIFO,
	 *	if we wakeup all waiting threads, one greedy thread
	 *	can starve multiple niceguy threads.  When the threads
	 *	all wakeup, the greedy threads runs first, grabs the page,
	 *	and waits for another page.  It will be the first to run
	 *	when the next page is freed.
	 *
	 *	However, there is a slight danger here.
	 *	The thread we wake might not use the free page.
	 *	Then the other threads could wait indefinitely
	 *	while the page goes unused.  To forestall this,
	 *	the pageout daemon will keep making free pages
	 *	as long as vm_page_free_wanted is non-zero.
	 */

	if ((vm_page_free_wanted > 0) &&
	    (vm_page_free_count >= vm_page_free_reserved)) {
		vm_page_free_wanted--;
		thread_wakeup_one((event_t) &vm_page_free_count);
	}

	mutex_unlock(&vm_page_queue_free_lock);
}

/*
 * Release a page to the limbo list.
 * Put real pages at the head of the queue, fictitious at the tail.
 * Page queues must be locked.
 */
void
vm_page_release_limbo(
	register vm_page_t	m)
{
	assert(m->limbo);
	vm_page_limbo_count++;
	if (m->fictitious) {
		queue_enter(&vm_page_queue_limbo, m, vm_page_t, pageq);
	} else {
		vm_page_limbo_real_count++;
		queue_enter_first(&vm_page_queue_limbo, m, vm_page_t, pageq);
	}
}

/*
 * Exchange a real page in limbo (limbo_m) with a fictitious page (new_m).
 * The end result is that limbo_m is fictitious and still in limbo, and new_m
 * is the real page.  The prep and pin counts remain with the page in limbo
 * although they will be briefly cleared by vm_page_init.  This is OK since
 * there will be no interrupt-level interactions (the page is in limbo) and
 * vm_page_unprep must lock the page queues before changing the prep count.
 *
 * Page queues must be locked, and limbo_m must have been removed from its
 * object.
 */
void
vm_page_limbo_exchange(
	register vm_page_t	limbo_m,
	register vm_page_t	new_m)
{
	assert(limbo_m->limbo && !limbo_m->fictitious);
	assert(!limbo_m->tabled);
	assert(new_m->fictitious);

	*new_m = *limbo_m;
	vm_page_init(limbo_m, vm_page_fictitious_addr);

	limbo_m->fictitious = TRUE;
	limbo_m->limbo = TRUE;
	new_m->limbo = FALSE;

	limbo_m->prep_pin_count = new_m->prep_pin_count;
	new_m->prep_pin_count = 0;
}

/*
 *	vm_page_wait:
 *
 *	Wait for a page to become available.
 *	If there are plenty of free pages, then we don't sleep.
 */

void
vm_page_wait( void )
{
	/*
	 *	We can't use vm_page_free_reserved to make this
	 *	determination.  Consider: some thread might
	 *	need to allocate two pages.  The first allocation
	 *	succeeds, the second fails.  After the first page is freed,
	 *	a call to vm_page_wait must really block.
	 */

	mutex_lock(&vm_page_queue_free_lock);
	if (vm_page_free_count < vm_page_free_target) {
		if (vm_page_free_wanted++ == 0)
			thread_wakeup((event_t)&vm_page_free_wanted);
		if (current_thread()->vm_privilege) {
			extern void assert_wait_first(event_t event,
						      boolean_t interruptible);
			/*
			 * a VM privileged thread blocking for a free page
			 * really needs to get a page soon: the other waiters
			 * can wait a little longer.
			 */
			assert_wait_first((event_t)&vm_page_free_count, FALSE);
		} else {
			assert_wait((event_t)&vm_page_free_count, FALSE);
		}
		mutex_unlock(&vm_page_queue_free_lock);
		counter(c_vm_page_wait_block++);
		thread_block((void (*)(void))0);
	} else
		mutex_unlock(&vm_page_queue_free_lock);
}

/*
 *	vm_page_alloc:
 *
 *	Allocate and return a memory cell associated
 *	with this VM object/offset pair.
 *
 *	Object must be locked.
 */

vm_page_t
vm_page_alloc(
	vm_object_t	object,
	vm_offset_t	offset)
{
	register vm_page_t	mem;

	mem = vm_page_grab();
	if (mem == VM_PAGE_NULL)
		return VM_PAGE_NULL;

	vm_page_insert(mem, object, offset);

	return(mem);
}

int c_limbo_page_free = 0;	/* debugging */
int c_limbo_convert = 0;	/* debugging */
counter(unsigned int c_laundry_pages_freed = 0;)

int vm_pagein_cluster_unused = 0;
boolean_t	vm_page_free_verify = FALSE;
/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page queues must be locked prior to entry.
 */
void
vm_page_free(
	register vm_page_t	mem)
{
	vm_object_t	object = mem->object;

	assert(!mem->free);
	assert(!mem->cleaning);
	assert(!mem->pageout);
	assert(!vm_page_free_verify || pmap_verify_free(mem->phys_addr));

	if (mem->tabled)
		vm_page_remove(mem);	/* clears tabled, object, offset */
	VM_PAGE_QUEUES_REMOVE(mem);	/* clears active or inactive */

	if (mem->clustered) {
		mem->clustered = FALSE;
		vm_pagein_cluster_unused++;
	}

	if (mem->wire_count) {
		if (!mem->private && !mem->fictitious)
			vm_page_wire_count--;
		mem->wire_count = 0;
		assert(!mem->gobbled);
	} else if (mem->gobbled) {
		if (!mem->private && !mem->fictitious)
			vm_page_wire_count--;
		vm_page_gobble_count--;
	}
	mem->gobbled = FALSE;

	if (mem->laundry) {
		extern int vm_page_laundry_min;
		vm_page_laundry_count--;
		mem->laundry = FALSE;	/* laundry is now clear */
		counter(++c_laundry_pages_freed);
		if (vm_page_laundry_count < vm_page_laundry_min) {
			vm_page_laundry_min = 0;
			thread_wakeup((event_t) &vm_page_laundry_count);
		}
	}

	mem->discard_request = FALSE;

	PAGE_WAKEUP(mem);	/* clears wanted */

	if (mem->absent)
		vm_object_absent_release(object);

	if (mem->limbo) {
		/*
		 * The pageout daemon put this page into limbo and then freed
		 * it.  The page has already been removed from the object and
		 * queues, so any attempt to look it up will fail.  Put it
		 * on the limbo queue; the pageout daemon will convert it to a
		 * fictitious page and/or free the real one later.
		 */
		/* assert that it came from pageout daemon (how?) */
		assert(!mem->fictitious && !mem->absent);
		c_limbo_page_free++;
		vm_page_release_limbo(mem);
		return;
	}
	assert(mem->prep_pin_count == 0);

		/* Some of these may be unnecessary */
	mem->page_lock = 0;
	mem->unlock_request = 0;
	mem->busy = TRUE;
	mem->absent = FALSE;
	mem->error = FALSE;
	mem->dirty = FALSE;
	mem->precious = FALSE;
	mem->reference = FALSE;

	mem->page_error = KERN_SUCCESS;

	if (mem->private) {
		mem->private = FALSE;
		mem->fictitious = TRUE;
		mem->phys_addr = vm_page_fictitious_addr;
	}
	if (mem->fictitious) {
		vm_page_release_fictitious(mem);
	} else {
		vm_page_init(mem, mem->phys_addr);
		vm_page_release(mem);
	}
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.
 *
 *	The page's object and the page queues must be locked.
 */
void
vm_page_wire(
	register vm_page_t	mem)
{
	VM_PAGE_CHECK(mem);

	if (mem->wire_count == 0) {
		VM_PAGE_QUEUES_REMOVE(mem);
		if (!mem->private && !mem->fictitious && !mem->gobbled)
			vm_page_wire_count++;
		if (mem->gobbled)
			vm_page_gobble_count--;
		mem->gobbled = FALSE;
	}
	assert(!mem->gobbled);
	mem->wire_count++;
}

/*
 *      vm_page_gobble:
 *
 *      Mark this page as consumed by the vm/ipc/xmm subsystems.
 *
 *      Called only for freshly vm_page_grab()ed pages - w/ nothing locked.
 */
void
vm_page_gobble(
        register vm_page_t      mem)
{
        vm_page_lock_queues();
        VM_PAGE_CHECK(mem);

	assert(!mem->gobbled);
	assert(mem->wire_count == 0);

        if (!mem->gobbled && mem->wire_count == 0) {
                if (!mem->private && !mem->fictitious)
                        vm_page_wire_count++;
        }
	vm_page_gobble_count++;
        mem->gobbled = TRUE;
        vm_page_unlock_queues();
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 *
 *	The page's object and the page queues must be locked.
 */
void
vm_page_unwire(
	register vm_page_t	mem)
{
	VM_PAGE_CHECK(mem);
	assert(mem->wire_count > 0);

	if (--mem->wire_count == 0) {
		assert(!mem->private && !mem->fictitious);
		vm_page_wire_count--;
		queue_enter(&vm_page_queue_active, mem, vm_page_t, pageq);
		vm_page_active_count++;
		mem->active = TRUE;
		mem->reference = TRUE;
	}
}

/*
 *	vm_page_deactivate:
 *
 *	Returns the given page to the inactive list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
void
vm_page_deactivate(
	register vm_page_t	m)
{
	VM_PAGE_CHECK(m);

	/*
	 *	This page is no longer very interesting.  If it was
	 *	interesting (active or inactive/referenced), then we
	 *	clear the reference bit and (re)enter it in the
	 *	inactive queue.  Note wired pages should not have
	 *	their reference bit cleared.
	 */
	if (m->gobbled) {		/* can this happen? */
		assert(m->wire_count == 0);
		if (!m->private && !m->fictitious)
			vm_page_wire_count--;
		vm_page_gobble_count--;
		m->gobbled = FALSE;
	}
	if (m->private || (m->wire_count != 0))
		return;
	if (m->active || (m->inactive && m->reference)) {
		if (!m->fictitious && !m->absent)
			pmap_clear_reference(m->phys_addr);
		m->reference = FALSE;
		VM_PAGE_QUEUES_REMOVE(m);
	}
	if (m->wire_count == 0 && !m->inactive) {
		queue_enter(&vm_page_queue_inactive, m, vm_page_t, pageq);
		m->inactive = TRUE;
		if (!m->fictitious)
			vm_page_inactive_count++;
	}
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	The page queues must be locked.
 */

void
vm_page_activate(
	register vm_page_t	m)
{
	VM_PAGE_CHECK(m);

	if (m->gobbled) {
		assert(m->wire_count == 0);
		if (!m->private && !m->fictitious)
			vm_page_wire_count--;
		vm_page_gobble_count--;
		m->gobbled = FALSE;
	}
	if (m->private)
		return;

	if (m->inactive) {
		queue_remove(&vm_page_queue_inactive, m, vm_page_t, pageq);
		if (!m->fictitious)
			vm_page_inactive_count--;
		m->inactive = FALSE;
	}
	if (m->wire_count == 0) {
		if (m->active)
			panic("vm_page_activate: already active");

		queue_enter(&vm_page_queue_active, m, vm_page_t, pageq);
		m->active = TRUE;
		m->reference = TRUE;
		if (!m->fictitious)
			vm_page_active_count++;
	}
}

/*
 *	vm_page_part_zero_fill:
 *
 *	Zero-fill a part of the page.
 */
void
vm_page_part_zero_fill(
	vm_page_t	m,
	vm_offset_t	m_pa,
	vm_size_t	len)
{
	VM_PAGE_CHECK(m);

	pmap_zero_part_page(m->phys_addr, m_pa, len);
}

/*
 *	vm_page_zero_fill:
 *
 *	Zero-fill the specified page.
 */
void
vm_page_zero_fill(
	vm_page_t	m)
{
        XPR(XPR_VM_PAGE,
                "vm_page_zero_fill, object 0x%X offset 0x%X page 0x%X\n",
                (integer_t)m->object, (integer_t)m->offset, (integer_t)m, 0,0);

	VM_PAGE_CHECK(m);

	pmap_zero_page(m->phys_addr);
}

/*
 *	vm_page_part_copy:
 *
 *	copy part of one page to another
 */

void
vm_page_part_copy(
	vm_page_t	src_m,
	vm_offset_t	src_pa,
	vm_page_t	dst_m,
	vm_offset_t	dst_pa,
	vm_size_t	len)
{
	VM_PAGE_CHECK(src_m);
	VM_PAGE_CHECK(dst_m);

	pmap_copy_part_page(src_m->phys_addr, src_pa,
			dst_m->phys_addr, dst_pa, len);
}

/*
 *	vm_page_copy:
 *
 *	Copy one page to another
 */

void
vm_page_copy(
	vm_page_t	src_m,
	vm_page_t	dest_m)
{
        XPR(XPR_VM_PAGE,
        "vm_page_copy, object 0x%X offset 0x%X to object 0x%X offset 0x%X\n",
        (integer_t)src_m->object, src_m->offset, 
	(integer_t)dest_m->object, dest_m->offset,
	0);

	VM_PAGE_CHECK(src_m);
	VM_PAGE_CHECK(dest_m);

	pmap_copy_page(src_m->phys_addr, dest_m->phys_addr);
}

/*
 * Limbo pages are placed on the limbo queue to await their prep count
 * going to zero.  A page is put into limbo by the pageout daemon.  If the
 * page is real, then the pageout daemon did not need to page out the page,
 * it just freed it.  When the prep_pin_count is zero the page can be freed.
 * Real pages with a non-zero prep count are converted to fictitious pages
 * so that the memory can be reclaimed; the fictitious page will remain on
 * the limbo queue until its prep count reaches zero.
 *
 * cleanup_limbo_queue is called by vm_page_more_fictitious and the pageout
 * daemon since it can free both real and fictitious pages.
 * It returns the number of fictitious pages freed.
 */
void
cleanup_limbo_queue(void)
{
	register vm_page_t free_m, m;
	vm_offset_t phys_addr;

	vm_page_lock_queues();
	assert(vm_page_limbo_count >= vm_page_limbo_real_count);

	/*
	 * first free up all pages with prep/pin counts of zero.  This
	 * may free both real and fictitious pages, which may be needed
	 * later to convert real ones.
	 */
	m = (vm_page_t)queue_first(&vm_page_queue_limbo);
	while (!queue_end(&vm_page_queue_limbo, (queue_entry_t)m)) {
		if (m->prep_pin_count == 0) {
			free_m = m;
			m = (vm_page_t)queue_next(&m->pageq);
			queue_remove(&vm_page_queue_limbo, free_m, vm_page_t,
									pageq);
			vm_page_limbo_count--;
			if (!free_m->fictitious)
				vm_page_limbo_real_count--;
			free_m->limbo = FALSE;
			vm_page_free(free_m);
			assert(vm_page_limbo_count >= 0);
			assert(vm_page_limbo_real_count >= 0);
		} else {
			m = (vm_page_t)queue_next(&m->pageq);
		}
	}

	/*
	 * now convert any remaining real pages to fictitious and free the
	 * real ones.
	 */
	while (vm_page_limbo_real_count > 0) {
		queue_remove_first(&vm_page_queue_limbo, m, vm_page_t, pageq);
		assert(!m->fictitious);
		assert(m->limbo);

		/*
		 * Try to get a fictitious page.  If impossible,
		 * requeue the real one and give up.
		 */
		free_m = vm_page_grab_fictitious();
		if (free_m == VM_PAGE_NULL) {
			queue_enter_first(&vm_page_queue_limbo, m, vm_page_t,
									pageq);
			break;
		}
		c_limbo_convert++;
		vm_page_limbo_exchange(m, free_m);
		assert(m->limbo && m->fictitious);
		assert(!free_m->limbo && !free_m->fictitious);
		queue_enter(&vm_page_queue_limbo, m, vm_page_t, pageq);
		vm_page_free(free_m);
		vm_page_limbo_real_count--;
	}

	vm_page_unlock_queues();
}

/*
 * Increment prep_count on a page.
 * Must be called in thread context.  Page must not disappear: object
 * must be locked.
 */
kern_return_t
vm_page_prep(
	register vm_page_t	m)
{
	kern_return_t retval = KERN_SUCCESS;
	int s;

	assert(m != VM_PAGE_NULL);
   	vm_page_lock_queues();
   	if (!m->busy && !m->error && !m->fictitious && !m->absent) {
   		if (m->prep_pin_count != 0) {
			s = splvm();
   			vm_page_pin_lock();
	   		m->prep_count++;
   			vm_page_pin_unlock();
			splx(s);
   		} else {
	   		m->prep_count++;
		}
		assert(m->prep_count != 0);	/* check for wraparound */
   	} else {
		retval = KERN_FAILURE;
	}
   	vm_page_unlock_queues();
	return retval;
}


/*
 * Pin a page (increment pin count).
 * Must have been previously prepped.
 *
 * MUST BE CALLED AT SPLVM.
 *
 * May be called from thread or interrupt context.
 * If page is in "limbo" it cannot be pinned.
 */
kern_return_t
vm_page_pin(
	register vm_page_t	m)
{
	kern_return_t retval = KERN_SUCCESS;

	assert(m != VM_PAGE_NULL);
	vm_page_pin_lock();
	if (m->limbo || m->prep_count == 0) {
		retval = KERN_FAILURE;
	} else {
		assert(!m->fictitious);
		if (m->pin_count == 0)
			vm_page_pin_count++;
		m->pin_count++;
	}
	vm_page_pin_unlock();
	return retval;
}


/*
 * Unprep a page (decrement prep count).
 * Must have been previously prepped.
 * Called to decrement prep count after an attempt to pin failed.
 * Must be called from thread context.
 */
kern_return_t
vm_page_unprep(
	register vm_page_t	m)
{
	kern_return_t retval = KERN_SUCCESS;
	int s;

	assert(m != VM_PAGE_NULL);
	vm_page_lock_queues();
	s = splvm();
	vm_page_pin_lock();
	assert(m->prep_count != 0);
	if (m->prep_count == 0)
		retval = KERN_FAILURE;		/* shouldn't happen */
	else
		m->prep_count--;
	vm_page_pin_unlock();
	splx(s);
	vm_page_unlock_queues();
	return retval;
}


/*
 * Unpin a page: decrement pin AND prep counts.
 * Must have been previously prepped AND pinned.
 *
 * MUST BE CALLED AT SPLVM.
 *
 * May be called from thread or interrupt context.
 */
kern_return_t
vm_page_unpin(
	register vm_page_t	m)
{
	kern_return_t retval = KERN_SUCCESS;

	assert(m != VM_PAGE_NULL);
	vm_page_pin_lock();
	assert(m->prep_count != 0 && m->pin_count != 0);
	assert(m->prep_count >= m->pin_count);
	assert(!m->limbo && !m->fictitious);
	if (m->prep_count != 0 && m->pin_count != 0) {
		m->prep_count--;
		m->pin_count--;
		if (m->pin_count == 0)
			vm_page_pin_count--;
	} else {
		retval = KERN_FAILURE;		/* shouldn't happen */
	}
	vm_page_pin_unlock();
	return retval;
}

/*
 *	Currently, this is a primitive allocator that grabs
 *	free pages from the system, sorts them by physical
 *	address, then searches for a region large enough to
 *	satisfy the user's request.
 *
 *	Additional levels of effort:
 *		+ steal clean active/inactive pages
 *		+ force pageouts of dirty pages
 *		+ maintain a map of available physical
 *		memory
 */

#define	SET_NEXT_PAGE(m,n)	((m)->pageq.next = (struct queue_entry *) (n))

#if	MACH_ASSERT
int	vm_page_verify_contiguous(
		vm_page_t	pages,
		unsigned int	npages);
#endif	/* MACH_ASSERT */

cpm_counter(unsigned int	vpfls_pages_handled = 0;)
cpm_counter(unsigned int	vpfls_head_insertions = 0;)
cpm_counter(unsigned int	vpfls_tail_insertions = 0;)
cpm_counter(unsigned int	vpfls_general_insertions = 0;)
cpm_counter(unsigned int	vpfc_failed = 0;)
cpm_counter(unsigned int	vpfc_satisfied = 0;)

/*
 *	Sort free list by ascending physical address,
 *	using a not-particularly-bright sort algorithm.
 *	Caller holds vm_page_queue_free_lock.
 */
static void
vm_page_free_list_sort(void)
{
	vm_page_t	sort_list;
	vm_page_t	sort_list_end;
	vm_page_t	m, m1, *prev, next_m;
	vm_offset_t	addr;
#if	MACH_ASSERT
	unsigned int	npages;
	int		old_free_count;
#endif	/* MACH_ASSERT */

#if	MACH_ASSERT
	/*
	 *	Verify pages in the free list..
	 */
	npages = 0;
	for (m = vm_page_queue_free; m != VM_PAGE_NULL; m = NEXT_PAGE(m))
		++npages;
	if (npages != vm_page_free_count)
		panic("vm_sort_free_list:  prelim:  npages %d free_count %d",
		      npages, vm_page_free_count);
	old_free_count = vm_page_free_count;
#endif	/* MACH_ASSERT */

	sort_list = sort_list_end = vm_page_queue_free;
	m = NEXT_PAGE(vm_page_queue_free);
	SET_NEXT_PAGE(vm_page_queue_free, VM_PAGE_NULL);
	cpm_counter(vpfls_pages_handled = 0);
	while (m != VM_PAGE_NULL) {
		cpm_counter(++vpfls_pages_handled);
		next_m = NEXT_PAGE(m);
		if (m->phys_addr < sort_list->phys_addr) {
			cpm_counter(++vpfls_head_insertions);
			SET_NEXT_PAGE(m, sort_list);
			sort_list = m;
		} else if (m->phys_addr > sort_list_end->phys_addr) {
			cpm_counter(++vpfls_tail_insertions);
			SET_NEXT_PAGE(sort_list_end, m);
			SET_NEXT_PAGE(m, VM_PAGE_NULL);
			sort_list_end = m;
		} else {
			cpm_counter(++vpfls_general_insertions);
			/* general sorted list insertion */
			prev = &sort_list;
			for (m1=sort_list; m1!=VM_PAGE_NULL; m1=NEXT_PAGE(m1)) {
				if (m1->phys_addr > m->phys_addr) {
					if (*prev != m1)
						panic("vm_sort_free_list: ugh");
					SET_NEXT_PAGE(m, *prev);
					*prev = m;
					break;
				}
				prev = (vm_page_t *) &m1->pageq.next;
			}
		}
		m = next_m;
	}

#if	MACH_ASSERT
	/*
	 *	Verify that pages are sorted into ascending order.
	 */
	for (m = sort_list, npages = 0; m != VM_PAGE_NULL; m = NEXT_PAGE(m)) {
		if (m != sort_list &&
		    m->phys_addr <= addr) {
			printf("m 0x%x addr 0x%x\n", m, addr);
			panic("vm_sort_free_list");
		}
		addr = m->phys_addr;
		++npages;
	}
	if (old_free_count != vm_page_free_count)
		panic("vm_sort_free_list:  old_free %d free_count %d",
		      old_free_count, vm_page_free_count);
	if (npages != vm_page_free_count)
		panic("vm_sort_free_list:  npages %d free_count %d",
		      npages, vm_page_free_count);
#endif	/* MACH_ASSERT */

	vm_page_queue_free = sort_list;
}


#if	MACH_ASSERT
/*
 *	Check that the list of pages is ordered by
 *	ascending physical address and has no holes.
 */
int
vm_page_verify_contiguous(
	vm_page_t	pages,
	unsigned int	npages)
{
	register vm_page_t	m;
	unsigned int		page_count;
	vm_offset_t		prev_addr;

	prev_addr = pages->phys_addr;
	page_count = 1;
	for (m = NEXT_PAGE(pages); m != VM_PAGE_NULL; m = NEXT_PAGE(m)) {
		if (m->phys_addr != prev_addr + page_size) {
			printf("m 0x%x prev_addr 0x%x, current addr 0x%x\n",
			       m, prev_addr, m->phys_addr);
			printf("pages 0x%x page_count %d\n", pages, page_count);
			panic("vm_page_verify_contiguous:  not contiguous!");
		}
		prev_addr = m->phys_addr;
		++page_count;
	}
	if (page_count != npages) {
		printf("pages 0x%x actual count 0x%x but requested 0x%x\n",
		       pages, page_count, npages);
		panic("vm_page_verify_contiguous:  count error");
	}
	return 1;
}
#endif	/* MACH_ASSERT */


/*
 *	Find a region large enough to contain at least npages
 *	of contiguous physical memory.
 *
 *	Requirements:
 *		- Called while holding vm_page_queue_free_lock.
 *		- Doesn't respect vm_page_free_reserved; caller
 *		must not ask for more pages than are legal to grab.
 *
 *	Returns a pointer to a list of gobbled pages or	VM_PAGE_NULL.
 *
 */
static vm_page_t
vm_page_find_contiguous(
	int		npages)
{
	vm_page_t	m, *contig_prev, *prev_ptr;
	vm_offset_t	prev_addr;
	unsigned int	contig_npages;
	vm_page_t	list;

	if (npages < 1)
		return VM_PAGE_NULL;

	prev_addr = vm_page_queue_free->phys_addr - (page_size + 1);
	prev_ptr = &vm_page_queue_free;
	for (m = vm_page_queue_free; m != VM_PAGE_NULL; m = NEXT_PAGE(m)) {

		if (m->phys_addr != prev_addr + page_size) {
			/*
			 *	Whoops!  Pages aren't contiguous.  Start over.
			 */
			contig_npages = 0;
			contig_prev = prev_ptr;
		}

		if (++contig_npages == npages) {
			/*
			 *	Chop these pages out of the free list.
			 *	Mark them all as gobbled.
			 */
			list = *contig_prev;
			*contig_prev = NEXT_PAGE(m);
			SET_NEXT_PAGE(m, VM_PAGE_NULL);
			for (m = list; m != VM_PAGE_NULL; m = NEXT_PAGE(m)) {
				assert(m->free);
				assert(!m->wanted);
				m->free = FALSE;
				m->gobbled = TRUE;
			}
			vm_page_free_count -= npages;
			if (vm_page_free_count < vm_page_free_count_minimum)
				vm_page_free_count_minimum = vm_page_free_count;
			vm_page_gobble_count += npages;
			cpm_counter(++vpfc_satisfied);
			assert(vm_page_verify_contiguous(list, contig_npages));
			return list;
		}

		assert(contig_npages < npages);
		prev_ptr = (vm_page_t *) &m->pageq.next;
		prev_addr = m->phys_addr;
	}
	cpm_counter(++vpfc_failed);
	return VM_PAGE_NULL;
}

/*
 *	Allocate a list of contiguous, wired pages.
 */
kern_return_t
cpm_allocate(
	vm_size_t	size,
	vm_page_t	*list,
	boolean_t	wire)
{
	register vm_page_t	m;
	vm_page_t		*first_contig;
	vm_page_t		free_list, pages;
	unsigned int		npages, n1pages;
	int			vm_pages_available;

	if (size % page_size != 0)
		return KERN_INVALID_ARGUMENT;

	vm_page_lock_queues();
	mutex_lock(&vm_page_queue_free_lock);

	/*
	 *	Should also take active and inactive pages
	 *	into account...  One day...
	 */
	vm_pages_available = vm_page_free_count - vm_page_free_reserved;

	if (size > vm_pages_available * page_size) {
		mutex_unlock(&vm_page_queue_free_lock);
		return KERN_RESOURCE_SHORTAGE;
	}

	vm_page_free_list_sort();

	npages = size / page_size;

	/*
	 *	Obtain a pointer to a subset of the free
	 *	list large enough to satisfy the request;
	 *	the region will be physically contiguous.
	 */
	pages = vm_page_find_contiguous(npages);
	if (pages == VM_PAGE_NULL) {
		mutex_unlock(&vm_page_queue_free_lock);
		return KERN_NO_SPACE;
	}

	mutex_unlock(&vm_page_queue_free_lock);

	/*
	 *	Walk the returned list, wiring the pages.
	 */
	if (wire == TRUE)
		for (m = pages; m != VM_PAGE_NULL; m = NEXT_PAGE(m)) {
			/*
			 *	Essentially inlined vm_page_wire.
			 */
			assert(!m->active);
			assert(!m->inactive);
			assert(!m->private);
			assert(!m->fictitious);
			assert(m->wire_count == 0);
			assert(m->gobbled);
			m->gobbled = FALSE;
			m->wire_count++;
			++vm_page_wire_count;
			--vm_page_gobble_count;
		}
	vm_page_unlock_queues();

	/*
	 *	The CPM pages should now be available and
	 *	ordered by ascending physical address.
	 */
	assert(vm_page_verify_contiguous(pages, npages));

	*list = pages;
	return KERN_SUCCESS;
}


#include <mach_vm_debug.h>
#if	MACH_VM_DEBUG

#include <mach_debug/hash_info.h>
#include <vm/vm_debug.h>

/*
 *	Routine:	vm_page_info
 *	Purpose:
 *		Return information about the global VP table.
 *		Fills the buffer with as much information as possible
 *		and returns the desired size of the buffer.
 *	Conditions:
 *		Nothing locked.  The caller should provide
 *		possibly-pageable memory.
 */

unsigned int
vm_page_info(
	hash_info_bucket_t *info,
	unsigned int count)
{
	int i;

	if (vm_page_bucket_count < count)
		count = vm_page_bucket_count;

	for (i = 0; i < count; i++) {
		vm_page_bucket_t *bucket = &vm_page_buckets[i];
		unsigned int bucket_count = 0;
		vm_page_t m;

		mutex_lock(&bucket->lock);
		for (m = bucket->pages; m != VM_PAGE_NULL; m = m->next)
			bucket_count++;
		mutex_unlock(&bucket->lock);

		/* don't touch pageable memory while holding locks */
		info[i].hib_count = bucket_count;
	}

	return vm_page_bucket_count;
}
#endif	/* MACH_VM_DEBUG */

#include <mach_kdb.h>
#if	MACH_KDB

#include <ddb/db_output.h>
#include <vm/vm_print.h>
#define	printf	kdbprintf

/*
 *	Routine:	vm_page_print [exported]
 */
void
vm_page_print(
	vm_page_t	p)
{
	extern int indent;

	iprintf("page 0x%x\n", p);

	indent += 2;

	iprintf("object=0x%x", p->object);
	printf(", offset=0x%x", p->offset);
	printf(", wire_count=%d", p->wire_count);
	printf(", prep_count=%d", p->prep_count);
	printf(", pin_count=%d\n", p->pin_count);

	iprintf("%sinactive, %sactive, %sgobbled, %slaundry, %sfree, %sref, %sdiscard\n",
		(p->inactive ? "" : "!"),
		(p->active ? "" : "!"),
		(p->gobbled ? "" : "!"),
		(p->laundry ? "" : "!"),
		(p->free ? "" : "!"),
		(p->reference ? "" : "!"),
		(p->discard_request ? "" : "!"));
	iprintf("%sbusy, %swanted, %stabled, %sfictitious, %sprivate, %sprecious\n",
		(p->busy ? "" : "!"),
		(p->wanted ? "" : "!"),
		(p->tabled ? "" : "!"),
		(p->fictitious ? "" : "!"),
		(p->private ? "" : "!"),
		(p->precious ? "" : "!"));
	iprintf("%sabsent, %serror, %sdirty, %scleaning, %spageout, %sclustered\n",
		(p->absent ? "" : "!"),
		(p->error ? "" : "!"),
		(p->dirty ? "" : "!"),
		(p->cleaning ? "" : "!"),
		(p->pageout ? "" : "!"),
		(p->clustered ? "" : "!"));
	iprintf("%slock_supplied, %soverwriting, %srestart, %sunusual, %slimbo\n",
		(p->lock_supplied ? "" : "!"),
		(p->overwriting ? "" : "!"),
		(p->restart ? "" : "!"),
		(p->unusual ? "" : "!"),
		(p->limbo ? "" : "!"));

	iprintf("phys_addr=0x%x", p->phys_addr);
	printf(", page_error=0x%x", p->page_error);
	printf(", page_lock=0x%x", p->page_lock);
	printf(", unlock_request=%d\n", p->unlock_request);

	indent -= 2;
}
#endif	/* MACH_KDB */
