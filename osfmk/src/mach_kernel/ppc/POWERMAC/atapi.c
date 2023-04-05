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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */
/*
 * Device-independent level for ATAPI drivers.
 *
 * Copyright (C) 1995 Cronyx Ltd.
 * Author Serge Vakulenko, <vak@cronyx.ru>
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organisations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * Version 1.9, Mon Oct  9 22:34:47 MSK 1995
 */

/*
 * The ATAPI level is implemented as a machine-dependent layer
 * between the device driver and the IDE controller.
 * All the machine- and controller dependency is isolated inside
 * the ATAPI level, while all the device dependency is located
 * in the device subdriver.
 *
 * It seems that an ATAPI bus will became popular for medium-speed
 * storage devices such as CD-ROMs, magneto-optical disks, tape streamers etc.
 *
 * To ease the development of new ATAPI drivers, the subdriver
 * interface was designed to be as simple as possible.
 *
 * Three routines are available for the subdriver to access the device:
 *
 *      struct atapires atapi_request_wait (ata, unit, cmd, a1, a2, a3, a4, a5,
 *              a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, addr, count);
 *      struct atapi *ata;  -- atapi controller descriptor
 *      int unit;           -- device unit number on the IDE bus
 *      u_char cmd;         -- ATAPI command code
 *      u_char a1..a15;     -- ATAPI command arguments
 *      char *addr;         -- address of the data buffer for i/o
 *      int count;          -- data length, >0 for read ops, <0 for write ops
 *
 * The atapi_request_wait() function puts the op in the queue of ATAPI
 * commands for the IDE controller, starts the controller, the waits for
 * operation to be completed (using tsleep).
 * The function should be called from the user phase only (open(), close(),
 * ioctl() etc).
 * Ata and unit args are the values which the subdriver gets from the ATAPI
 * level via attach() call.
 * Buffer pointed to by *addr should be placed in core memory, static
 * or dynamic, but not in stack.
 * The function returns the error code structure, which consists of:
 * - atapi driver code value
 * - controller status port value
 * - controller error port value
 *
 *      struct atapires atapi_request_immediate (ata, unit, cmd, a1, a2, a3,
 *              a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
 *              addr, count);
 *
 * The atapi_request_immediate() function is similar to atapi_request_wait(),
 * but it does not use interrupts for performing the request.
 * It should be used during an attach phase to get parameters from the device.
 *
 *      void atapi_request_callback (ata, unit, cmd, a1, a2, a3, a4, a5,
 *              a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
 *              addr, count, done, x, y);
 *      struct atapi *ata;  -- atapi controller descriptor
 *      int unit;           -- device unit number on the IDE bus
 *      u_char cmd;         -- ATAPI command code
 *      u_char a1..a15;     -- ATAPI command arguments
 *      char *addr;         -- address of the data buffer for i/o
 *      int count;          -- data length, >0 for read ops, <0 for write ops
 *      void (*done)();     -- function to call when op finished
 *      void *x, *y;        -- arguments for done() function
 *
 * The atapi_request_callback() function puts the op in the queue of ATAPI
 * commands for the IDE controller, starts the controller, then returns.
 * When the operation finishes, then the callback function done()
 * will be called on the interrupt level.
 * The function is designed to be callable from the interrupt phase.
 * The done() functions is called with the following arguments:
 *      (void) (*done) (x, y, count, errcode)
 *      void *x, *y;             -- arguments from the atapi_request_callback()
 *      int count;               -- the data residual count
 *      struct atapires errcode; -- error code structure, see above
 *
 * The new driver could be added in three steps:
 * 1. Add entries for the new driver to bdevsw and cdevsw tables in conf.c.
 *    You will need to make at least three routines: open(), close(),
 *    strategy() and possibly ioctl().
 * 2. Make attach() routine, which should allocate all the needed data
 *    structures and print the device description string (see wcdattach()).
 * 3. Add an appropriate case to the switch in atapi_attach() routine,
 *    call attach() routine of the new driver here.  Add the appropriate
 *    #include line at the top of attach.c.
 * That's all!
 *
 * Use #define DEBUG in atapi.c to enable tracing of all i/o operations
 * on the IDE bus.
 */
#undef DEBUG

#include "wd.h"

#if NWD > 0
#include <cpus.h>
#include <chained_ios.h>
#include <device/param.h>
 
#include <string.h> 
#include <types.h>
#include <device/buf.h>
#include <device/conf.h>
#include <device/errno.h>
#include <device/subrs.h> 
#include <device/misc_protos.h>
#include <device/ds_routines.h>
#include <device/param.h>
#include <device/driver_lock.h>
#include <sys/ioctl.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <machine/disk.h>
#include <chips/busses.h>
#include <vm/vm_kern.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/wdreg.h>
#include <ppc/POWERMAC/wd_entries.h>
#include <ppc/POWERMAC/dbdma.h>
#include <ppc/POWERMAC/atapi.h>


#define	DEV_BSIZE	512

#define	DELAY(x)	delay(x)

static void __inline__
outb(unsigned long addr, volatile unsigned char value)
{
	*(volatile unsigned char *) addr  = value;
	eieio();
}

static volatile unsigned char __inline__
inb(unsigned long addr)
{
	volatile unsigned char value;

	value = *(volatile unsigned char *) addr;
	eieio();

	return value;
}

static void volatile __inline__
outw(unsigned long addr, volatile unsigned short value)
{
	*(volatile unsigned short *) addr = value; eieio();

}

static volatile unsigned short __inline__
inw_le(unsigned long addr)
{
	volatile unsigned short value;

	asm volatile("lhbrx %0,0,%1" : "=r" (value) : "r" (addr)); eieio();

	return value;
}

static volatile unsigned short __inline__
inw(unsigned long addr)
{
	volatile unsigned short value;

	value = *(volatile unsigned short *) addr; eieio();

	return value;
}

static void __inline__
outsw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		outw(addr, *value++);
}

static void __inline__
insw_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw_le(addr);
}

static void __inline
insw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw(addr);
}

static void __inline__
outsl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*(volatile unsigned long  *) addr = *value++; eieio();
	}
}

static void __inline__
insl_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		asm volatile("lwbrx %0,0,%1" : "=r" (*value) : "r" (addr));
		eieio();
		value++;
	}
}

static void __inline__
insl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*value++ = *(volatile unsigned long *)addr;
		eieio();
	}
}

#ifndef ATAPI_STATIC
/* this code is compiled as part of the kernel if options ATAPI */
/*
 * In the case of loadable ATAPI driver we need to store
 * the probe info for delayed attaching.
 */
struct atapidrv atapi_drvtab[4];
int atapi_ndrv;
struct atapi *atapi_tab;

int atapi_attach (int ctlr, int unit, int port)
{
	atapi_drvtab[atapi_ndrv].ctlr     = ctlr;
	atapi_drvtab[atapi_ndrv].unit     = unit;
	atapi_drvtab[atapi_ndrv].port     = port;
	atapi_drvtab[atapi_ndrv].attached = 0;
	++atapi_ndrv;
	return (1);
}
#else /* ATAPI_STATIC */
/* this code is compiled part of the module */

#ifdef DEBUG
#   define print(s)     printf s
#else
#   define print(s)     {/*void*/}
#endif

/*
 * ATAPI packet command phase.
 */
#define PHASE_CMDOUT    (ARS_DRQ | ARI_CMD)
#define PHASE_DATAIN    (ARS_DRQ | ARI_IN)
#define PHASE_DATAOUT   ARS_DRQ
#define PHASE_COMPLETED (ARI_IN | ARI_CMD)
#define PHASE_ABORTED   0                       /* nonstandard - for NEC 260 */

struct atapi atapitab[NWD];

static struct atapi_params *atapi_probe (int port, int unit);
static int atapi_wait (int port, u_char bits_wanted);
static void atapi_send_cmd (struct atapi *ata, struct atapicmd *ac);
static int atapi_io (struct atapi *ata, struct atapicmd *ac);
static int atapi_start_cmd (struct atapi *ata, struct atapicmd *ac);
static int atapi_wait_cmd (struct atapi *ata, struct atapicmd *ac);

extern int wdstart (int ctrlr);
extern int wcdattach(struct atapi*, int, struct atapi_params*, int);
static void btrim (char *buf, int len);

/*
 * Probe the ATAPI device at IDE controller `ctlr', drive `unit'.
 * Called at splbio().
 */
#ifdef ATAPI_MODULE
static
#endif
int atapi_attach (int ctlr, int unit, int port)
{
	struct atapi *ata = atapitab + ctlr;
	struct atapi_params *ap;
	char buf [sizeof(ap->model) + 1];
	char revbuf [sizeof(ap->revision) + 1];
	struct atapicmd *ac;

	print (("atapi%d.%d at 0x%x: attach called\n", ctlr, unit, port));
	ap = atapi_probe (port, unit);
	if (! ap)
		return (0);

	bcopy (ap->model, buf, sizeof(buf)-1);
	buf[sizeof(buf)-1] = 0;

	bcopy (ap->revision, revbuf, sizeof(revbuf)-1);
	revbuf[sizeof(revbuf)-1] = 0;

	printf ("wdc%d: unit %d (atapi): <%s/%s>", ctlr, unit, buf, revbuf);

	/* device is removable */
	if (ap->removable)
		printf (", removable");

	/* packet command size */
	switch (ap->cmdsz) {
	case AT_PSIZE_12: break;
	case AT_PSIZE_16: printf (", cmd16"); ata->cmd16 = 1; break;
	default:          printf (", cmd%d", ap->cmdsz);
	}

	/* DRQ type */
	switch (ap->drqtype) {
	case AT_DRQT_MPROC: ata->slow = 1; break;
	case AT_DRQT_INTR:  printf (", intr"); ata->intrcmd = 1; break;
	case AT_DRQT_ACCEL: printf (", accel"); break;
	default:            printf (", drq%d", ap->drqtype);
	}

	/* overlap operation supported */
	if (ap->ovlapflag)
		printf (", ovlap");

	/* interleaved DMA supported */
	if (ap->idmaflag)
		printf (", idma");
	/* DMA supported */
	else if (ap->dmaflag)
		printf (", dma");

	/* IORDY can be disabled */
	if (ap->iordydis)
		printf (", iordis");
	/* IORDY supported */
	else if (ap->iordyflag)
		printf (", iordy");

	printf ("\n");

	ata->port = port;
	ata->ctrlr = ctlr;
	ata->attached[unit] = 0;
#ifdef DEBUG
	ata->debug = 1;
#else
	ata->debug = 0;
#endif
	/* Initialize free queue. */
	ata->cmdrq[15].next = 0;
	for (ac = ata->cmdrq+14; ac >= ata->cmdrq; --ac)
		ac->next = ac+1;
	ata->free = ata->cmdrq;

	if (ap->proto != AT_PROTO_ATAPI) {
		printf ("wdc%d: unit %d: unknown ATAPI protocol=%d\n",
			ctlr, unit, ap->proto);
		kmem_free(kernel_map, (vm_offset_t) ap, sizeof(*ap));
		return (0);
	}


	ata->params[unit] = ap;
#ifdef ATAPI_MODULE
	return (1);
#else
	switch (ap->devtype) {
	default:
		/* unknown ATAPI device */
		printf ("wdc%d: unit %d: unknown ATAPI type=%d\n",
			ctlr, unit, ap->devtype);
		break;

	case AT_TYPE_DIRECT:            /* direct-access */
	case AT_TYPE_CDROM:             /* CD-ROM device */
		break;

	case AT_TYPE_TAPE:              /* streaming tape (QIC-121 model) */
#if NWMT > 0
		/* Add your driver here */
#else
		printf ("wdc%d: ATAPI streaming tapes not supported yet\n", ctlr);
#endif
		break;

	case AT_TYPE_OPTICAL:           /* optical disk */
#if NWMD > 0
		/* Add your driver here */
#else
		printf ("wdc%d: ATAPI optical disks not supported yet\n", ctlr);
#endif
		break;
	}

	return (1);
#endif /* ATAPI_MODULE */
}

static char *cmdname (u_char cmd)
{
	static char buf[8];

	switch (cmd) {
	case 0x00: return ("TEST_UNIT_READY");
	case 0x03: return ("REQUEST_SENSE");
	case 0x1b: return ("START_STOP");
	case 0x1e: return ("PREVENT_ALLOW");
	case 0x25: return ("READ_CAPACITY");
	case 0x28: return ("READ_BIG");
	case 0x43: return ("READ_TOC");
	case 0x42: return ("READ_SUBCHANNEL");
	case 0x55: return ("MODE_SELECT_BIG");
	case 0x5a: return ("MODE_SENSE");
	case 0xb4: return ("PLAY_CD");
	case 0x47: return ("PLAY_MSF");
	case 0x4b: return ("PAUSE");
	case 0x48: return ("PLAY_TRACK");
	case 0xa5: return ("PLAY_BIG");
	}
	sprintf (buf, "[0x%x]", cmd);
	return (buf);
}

static void btrim (char *buf, int len)
{
	char *p;

	/* Remove the trailing spaces. */
	for (p=buf; p<buf+len; ++p)
		if (! *p)
			*p = ' ';
	for (p=buf+len-1; p>=buf && *p==' '; --p)
		*p = 0;
}

/*
 * Issue IDENTIFY command to ATAPI drive to ask it what it is.
 */
static struct atapi_params *atapi_probe (int port, int unit)
{
	struct atapi_params *ap;
	char tb [DEV_BSIZE];
#ifdef PC98
	int cnt;

	outb(0x432,unit%2); 
	print(("unit = %d,select %d\n",unit,unit%2)); 
#endif
	/* Wait for controller not busy. */
#ifdef PC98
	outb (port + AR_DRIVE, unit / 2 ? ARD_DRIVE1 : ARD_DRIVE0);
#else
	outb (port + AR_DRIVE, unit ? ARD_DRIVE1 : ARD_DRIVE0);
#endif
	if (atapi_wait (port, 0) < 0) {
		print (("atapiX.%d at 0x%x: controller busy, status=%b\n",
			unit, port, inb (port + AR_STATUS), ARS_BITS));
		return (0);
	}

	/* Issue ATAPI IDENTIFY command. */
#ifdef PC98
	outb (port + AR_DRIVE, unit/2 ? ARD_DRIVE1 : ARD_DRIVE0);

	/* Wait for DRQ deassert. */
	for (cnt=2000; cnt>0; --cnt)
	  if (! (inb (0x640 + AR_STATUS) & ARS_DRQ))
	    break;

	outb (port + AR_COMMAND, ATAPIC_IDENTIFY);
	DELAY(500);
#else
	outb (port + AR_DRIVE, unit ? ARD_DRIVE1 : ARD_DRIVE0);
	outb (port + AR_COMMAND, ATAPIC_IDENTIFY);
#endif

	/* Check that device is present. */
	if (inb (port + AR_STATUS) == 0xff) {
		print (("atapiX.%d at 0x%x: no device\n", unit, port));
		if (unit == 1)
			/* Select unit 0. */
			outb (port + AR_DRIVE, ARD_DRIVE0);
		return (0);
	}

	/* Wait for data ready. */
	if (atapi_wait (port, ARS_DRQ) != 0) {
		print (("atapiX.%d at 0x%x: identify not ready, status=%b\n",
			unit, port, inb (port + AR_STATUS), ARS_BITS));
		if (unit == 1)
			/* Select unit 0. */
			outb (port + AR_DRIVE, ARD_DRIVE0);
		return (0);
	}

	/* Obtain parameters. */
	insw_le(port + AR_DATA, tb, sizeof(tb) / sizeof(short));

	if (kmem_alloc(kernel_map, (vm_offset_t *) &ap, sizeof(*ap)) != KERN_SUCCESS)
		return 0;

	bcopy (tb, (char *)ap, sizeof *ap);

	/*
	 * Shuffle string byte order.
	 * Mitsumi and NEC drives don't need this.
	 */
#if 0
	if (! ((ap->model[0] == 'N' && ap->model[1] == 'E') ||
	    (ap->model[0] == 'F' && ap->model[1] == 'X')))
		bswap (ap->model, sizeof(ap->model));
#endif

	/* Clean up the model name, serial and revision numbers. */
	btrim (ap->model, sizeof(ap->model));
	btrim (ap->serial, sizeof(ap->serial));
	btrim (ap->revision, sizeof(ap->revision));
	return (ap);
}

/*
 * Wait uninterruptibly until controller is not busy and certain
 * status bits are set.
 * The wait is usually short unless it is for the controller to process
 * an entire critical command.
 * Return 1 for (possibly stale) controller errors, -1 for timeout errors,
 * or 0 for no errors.
 */
static int atapi_wait (int port, u_char bits_wanted)
{
	int cnt;
	u_char s;

	/* Wait 5 sec for BUSY deassert. */
	for (cnt=500000; cnt>0; --cnt) {
		s = inb (port + AR_STATUS);
		if (! (s & ARS_BSY))
			break;
		DELAY (10);
	}
	if (cnt <= 0)
		return (-1);
	if (! bits_wanted)
		return (s & ARS_CHECK);

	/* Wait 50 msec for bits wanted. */
	for (cnt=5000; cnt>0; --cnt) {
		s = inb (port + AR_STATUS);
		if ((s & bits_wanted) == bits_wanted)
			return (s & ARS_CHECK);
		DELAY (10);
	}
	return (-1);
}

void atapi_debug (struct atapi *ata, int on)
{
	ata->debug = on;
}

static struct atapicmd *atapi_alloc (struct atapi *ata)
{
	struct atapicmd *ac;

	while (! ata->free) {
		thread_set_timeout(1);
		thread_block((void (*)(void)) 0);
		reset_timeout_check(&current_thread()->timer);
	}

	ac = ata->free;
	ata->free = ac->next;
	ac->busy = 1;
	return (ac);
}

static void atapi_free (struct atapi *ata, struct atapicmd *ac)
{
#if 0
	if (! ata->free)
		wakeup ((caddr_t)&ata);
#endif

	ac->busy = 0;
	ac->next = ata->free;
	ata->free = ac;
}

/*
 * Add new command request to the end of the queue.
 */
static void atapi_enqueue (struct atapi *ata, struct atapicmd *ac)
{
	ac->next = 0;
	if (ata->tail)
		ata->tail->next = ac;
	else
		ata->queue = ac;
	ata->tail = ac;
}

static void atapi_done (struct atapi *ata)
{
	struct atapicmd *ac = ata->queue;

	if (! ac)
		return; /* cannot happen */

	ata->queue = ac->next;
	if (! ata->queue)
		ata->tail = 0;

	if (ac->callback) {
		(*ac->callback) (ac->cbarg1, ac->cbarg2, ac->count, ac->result);
		atapi_free (ata, ac);
	} else
		thread_wakeup ((event_t)ac);
}

/*
 * Start new packet op.  Called from wdstart().
 * Return 1 if op started, and we are waiting for interrupt.
 * Return 0 when idle.
 */
int atapi_start (int ctrlr)
{
	struct atapi *ata = atapitab + ctrlr;
	struct atapicmd *ac;
again:
	ac = ata->queue;
	if (! ac)
		return (0);

	/* Start packet command. */
	if (atapi_start_cmd (ata, ac) < 0) {
		atapi_done (ata);
		goto again;
	}

	if (ata->intrcmd)
		/* Wait for interrupt before sending packet command */
		return (1);

	/* Wait for DRQ. */
	if (atapi_wait_cmd (ata, ac) < 0) {
		atapi_done (ata);
		goto again;
	}

	/* Send packet command. */
	atapi_send_cmd (ata, ac);
	return (1);
}

/*
 * Start new packet op. Returns -1 on errors.
 */
int atapi_start_cmd (struct atapi *ata, struct atapicmd *ac)
{
	ac->result.error = 0;
	ac->result.status = 0;

#ifdef PC98
	outb(0x432,(ac->unit)%2); 
	print(("(ac->unit) = %d,select %d (2) \n",(ac->unit),(ac->unit)%2)); 
	outb (ata->port + AR_DRIVE, (ac->unit)/2 ? ARD_DRIVE1 : ARD_DRIVE0);
#else
	outb (ata->port + AR_DRIVE, ac->unit ? ARD_DRIVE1 : ARD_DRIVE0);
#endif
	if (atapi_wait (ata->port, 0) < 0) {
		printf ("atapi%d.%d: controller not ready for cmd\n",
			ata->ctrlr, ac->unit);
		ac->result.code = RES_NOTRDY;
		return (-1);
	}

	/* Set up the controller registers. */
	outb (ata->port + AR_FEATURES, 0);
	outb (ata->port + AR_IREASON, 0);
	outb (ata->port + AR_TAG, 0);
	outb (ata->port + AR_CNTLO, ac->count & 0xff);
	outb (ata->port + AR_CNTHI, ac->count >> 8);
	outb (ata->port + AR_COMMAND, ATAPIC_PACKET);

	if (ata->debug)
		printf ("atapi%d.%d: start\n", ata->ctrlr, ac->unit);
	return (0);
}

/*
 * Wait for DRQ before sending packet cmd. Returns -1 on errors.
 */
int atapi_wait_cmd (struct atapi *ata, struct atapicmd *ac)
{
	/* Wait for DRQ from 50 usec to 3 msec for slow devices */
	int cnt = ata->intrcmd ? 10000 : ata->slow ? 3000 : 50;
	int ireason = 0, phase = 0;

	/* Wait for command phase. */
	for (; cnt>0; cnt-=10) {
		ireason = inb (ata->port + AR_IREASON);
		ac->result.status = inb (ata->port + AR_STATUS);
		phase = (ireason & (ARI_CMD | ARI_IN)) |
			(ac->result.status & (ARS_DRQ | ARS_BSY));
		if (phase == PHASE_CMDOUT)
			break;
		DELAY (10);
	}

	if (phase != PHASE_CMDOUT) {
		ac->result.code = RES_NODRQ;
		ac->result.error = inb (ata->port + AR_ERROR);
		printf ("atapi%d.%d: invalid command phase, ireason=0x%x, status=%b, error=%b\n",
			ata->ctrlr, ac->unit, ireason,
			ac->result.status, ARS_BITS,
			ac->result.error, AER_BITS);
		return (-1);
	}
	return (0);
}

/*
 * Send packet cmd.
 */
void atapi_send_cmd (struct atapi *ata, struct atapicmd *ac)
{
	outsw (ata->port + AR_DATA, ac->cmd, ata->cmd16 ? 8 : 6);
	if (ata->debug)
		printf ("atapi%d.%d: send cmd %s %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x\n",
			ata->ctrlr, ac->unit, cmdname (ac->cmd[0]), ac->cmd[0],
			ac->cmd[1], ac->cmd[2], ac->cmd[3], ac->cmd[4],
			ac->cmd[5], ac->cmd[6], ac->cmd[7], ac->cmd[8],
			ac->cmd[9], ac->cmd[10], ac->cmd[11], ac->cmd[12],
			ac->cmd[13], ac->cmd[14], ac->cmd[15]);
}

/*
 * Interrupt routine for the controller.  Called from wdintr().
 * Finish the started op, wakeup wait-type commands,
 * run callbacks for callback-type commands, then return.
 * Do not start new op here, it will be done by wdstart,
 * which is called just after us.
 * Return 1 if op continues, and we are waiting for new interrupt.
 * Return 0 when idle.
 */
int atapi_intr (int ctrlr)
{
	struct atapi *ata = atapitab + ctrlr;
	struct atapicmd *ac = ata->queue;

#ifdef PC98
	outb(0x432,(ac->unit)%2);
	print(("atapi_intr:(ac->unit)= %d,select %d\n",ac->unit,(ac->unit)%2));
#endif

	if (! ac) {
		printf ("atapi%d: stray interrupt\n", ata->ctrlr);
		return (0);
	}
	if (atapi_io (ata, ac) > 0)
		return (1);
	atapi_done (ata);
	return (0);
}

/*
 * Process the i/o phase, transferring the command/data to/from the device.
 * Return 1 if op continues, and we are waiting for new interrupt.
 * Return 0 when idle.
 */
int atapi_io (struct atapi *ata, struct atapicmd *ac)
{
	u_char ireason;
	u_short len, i;

	if (atapi_wait (ata->port, 0) < 0) {
		ac->result.status = inb (ata->port + AR_STATUS);
		ac->result.error = inb (ata->port + AR_ERROR);
		ac->result.code = RES_NOTRDY;
		printf ("atapi%d.%d: controller not ready, status=%b, error=%b\n",
			ata->ctrlr, ac->unit, ac->result.status, ARS_BITS,
			ac->result.error, AER_BITS);
		return (0);
	}

	ac->result.status = inb (ata->port + AR_STATUS);
	ac->result.error = inb (ata->port + AR_ERROR);
	len = inb (ata->port + AR_CNTLO);
	len |= inb (ata->port + AR_CNTHI) << 8;
	ireason = inb (ata->port + AR_IREASON);

	if (ata->debug) {
		printf ("atapi%d.%d: intr ireason=0x%x, len=%d, status=%b, error=%b\n",
			ata->ctrlr, ac->unit, ireason, len,
			ac->result.status, ARS_BITS,
			ac->result.error, AER_BITS);

	}
	switch ((ireason & (ARI_CMD | ARI_IN)) | (ac->result.status & ARS_DRQ)) {
	default:
		printf ("atapi%d.%d: unknown phase\n", ata->ctrlr, ac->unit);
		ac->result.code = RES_ERR;
		break;

	case PHASE_CMDOUT:
		/* Send packet command. */
		if (! (ac->result.status & ARS_DRQ)) {
			printf ("atapi%d.%d: no cmd drq\n",
				ata->ctrlr, ac->unit);
			ac->result.code = RES_NODRQ;
			break;
		}
		atapi_send_cmd (ata, ac);
		return (1);

	case PHASE_DATAOUT:
		/* Write data */
		if (ac->count > 0) {
			printf ("atapi%d.%d: invalid data direction\n",
				ata->ctrlr, ac->unit);
			ac->result.code = RES_INVDIR;
			break;
		}
		if (-ac->count < len) {
			print (("atapi%d.%d: send data underrun, %d bytes left\n",
				ata->ctrlr, ac->unit, -ac->count));
			ac->result.code = RES_UNDERRUN;
			outsw (ata->port + AR_DATA, ac->addr,
				-ac->count / sizeof(short));
			for (i= -ac->count; i<len; i+=sizeof(short))
				outw (ata->port + AR_DATA, 0);
		} else
			outsw (ata->port + AR_DATA, ac->addr,
				len / sizeof(short));
		ac->addr += len;
		ac->count += len;
		return (1);

	case PHASE_DATAIN:
		/* Read data */
		if (ac->count < 0) {
			printf ("atapi%d.%d: invalid data direction\n",
				ata->ctrlr, ac->unit);
			ac->result.code = RES_INVDIR;
			break;
		}
		if (ac->count < len) {
			print (("atapi%d.%d: recv data overrun, %d bytes left\n",
				ata->ctrlr, ac->unit, ac->count));
			ac->result.code = RES_OVERRUN;
			insw (ata->port + AR_DATA, ac->addr,
				ac->count / sizeof(short));
			for (i=ac->count; i<len; i+=sizeof(short))
				inw (ata->port + AR_DATA);
		} else
			insw (ata->port + AR_DATA, ac->addr,
				len / sizeof(short));
		ac->addr += len;
		ac->count -= len;
		return (1);

	case PHASE_ABORTED:
	case PHASE_COMPLETED:
		if (ac->result.status & (ARS_CHECK | ARS_DF))
			ac->result.code = RES_ERR;
		else if (ac->count < 0) {
			print (("atapi%d.%d: send data overrun, %d bytes left\n",
				ata->ctrlr, ac->unit, -ac->count));
			ac->result.code = RES_OVERRUN;
		} else if (ac->count > 0) {
			print (("atapi%d.%d: recv data underrun, %d bytes left\n",
				ata->ctrlr, ac->unit, ac->count));
			ac->result.code = RES_UNDERRUN;
			bzero (ac->addr, ac->count);
		} else
			ac->result.code = RES_OK;
		break;
	}
	return (0);
}

/*
 * Queue new packet request, then call wdstart().
 * Called on splbio().
 */
void atapi_request_callback (struct atapi *ata, int unit,
	u_char cmd, u_char a1, u_char a2, u_char a3, u_char a4,
	u_char a5, u_char a6, u_char a7, u_char a8, u_char a9,
	u_char a10, u_char a11, u_char a12, u_char a13, u_char a14, u_char a15,
	char *addr, int count, atapi_callback_t *done, void *x, void *y)
{
	struct atapicmd *ac;

	ac = atapi_alloc (ata);
	ac->cmd[0] = cmd;       ac->cmd[1] = a1;
	ac->cmd[2] = a2;        ac->cmd[3] = a3;
	ac->cmd[4] = a4;        ac->cmd[5] = a5;
	ac->cmd[6] = a6;        ac->cmd[7] = a7;
	ac->cmd[8] = a8;        ac->cmd[9] = a9;
	ac->cmd[10] = a10;      ac->cmd[11] = a11;
	ac->cmd[12] = a12;      ac->cmd[13] = a13;
	ac->cmd[14] = a14;      ac->cmd[15] = a15;
	ac->unit = unit;
	ac->addr = addr;
	ac->count = count;
	ac->callback = done;
	ac->cbarg1 = x;
	ac->cbarg2 = y;

	if (ata->debug)
		printf ("atapi%d.%d: req cb %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x len=%d\n",
			ata->ctrlr, ac->unit, ac->cmd[0], ac->cmd[1],
			ac->cmd[2], ac->cmd[3], ac->cmd[4], ac->cmd[5],
			ac->cmd[6], ac->cmd[7], ac->cmd[8], ac->cmd[9],
			ac->cmd[10], ac->cmd[11], ac->cmd[12],
			ac->cmd[13], ac->cmd[14], ac->cmd[15], count);
	atapi_enqueue (ata, ac);
	wdstart (ata->ctrlr);
}

/*
 * Queue new packet request, then call wdstart().
 * Wait until the request is finished.
 * Called on spl0().
 * Return atapi error.
 * Buffer pointed to by *addr should be placed in core memory, not in stack!
 */
struct atapires atapi_request_wait (struct atapi *ata, int unit,
	u_char cmd, u_char a1, u_char a2, u_char a3, u_char a4,
	u_char a5, u_char a6, u_char a7, u_char a8, u_char a9,
	u_char a10, u_char a11, u_char a12, u_char a13, u_char a14, u_char a15,
	char *addr, int count)
{
	struct atapicmd *ac;
	int x = splbio ();
	struct atapires result;

	ac = atapi_alloc (ata);
	ac->cmd[0] = cmd;       ac->cmd[1] = a1;
	ac->cmd[2] = a2;        ac->cmd[3] = a3;
	ac->cmd[4] = a4;        ac->cmd[5] = a5;
	ac->cmd[6] = a6;        ac->cmd[7] = a7;
	ac->cmd[8] = a8;        ac->cmd[9] = a9;
	ac->cmd[10] = a10;      ac->cmd[11] = a11;
	ac->cmd[12] = a12;      ac->cmd[13] = a13;
	ac->cmd[14] = a14;      ac->cmd[15] = a15;
	ac->unit = unit;
	ac->addr = addr;
	ac->count = count;
	ac->callback = 0;
	ac->cbarg1 = 0;
	ac->cbarg2 = 0;
	simple_lock_init(&ac->sleep_lock, ETAP_IO_DEV);

	if (ata->debug)
		printf ("atapi%d.%d: req w %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x len=%d\n",
			ata->ctrlr, ac->unit, ac->cmd[0], ac->cmd[1],
			ac->cmd[2], ac->cmd[3], ac->cmd[4], ac->cmd[5],
			ac->cmd[6], ac->cmd[7], ac->cmd[8], ac->cmd[9],
			ac->cmd[10], ac->cmd[11], ac->cmd[12],
			ac->cmd[13], ac->cmd[14], ac->cmd[15], count);
	simple_lock(&ac->sleep_lock);
	atapi_enqueue (ata, ac);
	wdstart (ata->ctrlr);
	thread_sleep_simple_lock((event_t) ac, simple_lock_addr(ac->sleep_lock), TRUE);
	result = ac->result;
	atapi_free (ata, ac);
	splx (x);
	return (result);
}

/*
 * Perform a packet command on the device.
 * Should be called on splbio().
 * Return atapi error.
 */
struct atapires atapi_request_immediate (struct atapi *ata, int unit,
	u_char cmd, u_char a1, u_char a2, u_char a3, u_char a4,
	u_char a5, u_char a6, u_char a7, u_char a8, u_char a9,
	u_char a10, u_char a11, u_char a12, u_char a13, u_char a14, u_char a15,
	char *addr, int count)
{
	struct atapicmd cmdbuf, *ac = &cmdbuf;
	int cnt;

	ac->cmd[0] = cmd;       ac->cmd[1] = a1;
	ac->cmd[2] = a2;        ac->cmd[3] = a3;
	ac->cmd[4] = a4;        ac->cmd[5] = a5;
	ac->cmd[6] = a6;        ac->cmd[7] = a7;
	ac->cmd[8] = a8;        ac->cmd[9] = a9;
	ac->cmd[10] = a10;      ac->cmd[11] = a11;
	ac->cmd[12] = a12;      ac->cmd[13] = a13;
	ac->cmd[14] = a14;      ac->cmd[15] = a15;
	ac->unit = unit;
	ac->addr = addr;
	ac->count = count;
	ac->callback = 0;
	ac->cbarg1 = 0;
	ac->cbarg2 = 0;

	if (ata->debug)
		printf ("atapi%d.%d: req im %x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x-%x len=%d\n",
			ata->ctrlr, ac->unit, ac->cmd[0], ac->cmd[1],
			ac->cmd[2], ac->cmd[3], ac->cmd[4], ac->cmd[5],
			ac->cmd[6], ac->cmd[7], ac->cmd[8], ac->cmd[9],
			ac->cmd[10], ac->cmd[11], ac->cmd[12],
			ac->cmd[13], ac->cmd[14], ac->cmd[15], count);

	/* Start packet command, wait for DRQ. */
	if (atapi_start_cmd (ata, ac) >= 0 && atapi_wait_cmd (ata, ac) >= 0) {
		/* Send packet command. */
		atapi_send_cmd (ata, ac);

		/* Wait for data i/o phase. */
		for (cnt=20000; cnt>0; --cnt)
			if (((inb (ata->port + AR_IREASON) & (ARI_CMD | ARI_IN)) |
			    (inb (ata->port + AR_STATUS) & ARS_DRQ)) != PHASE_CMDOUT)
				break;

		/* Do all needed i/o. */
		while (atapi_io (ata, ac))
			/* Wait for DRQ deassert. */
			for (cnt=2000; cnt>0; --cnt) {
				if (! (inb (ata->port + AR_STATUS) & ARS_DRQ))
					break;
				DELAY(10);
			}
	}
	return (ac->result);
}
#endif /* ATAPI_STATIC */
#endif /* NWD && ATAPI */
