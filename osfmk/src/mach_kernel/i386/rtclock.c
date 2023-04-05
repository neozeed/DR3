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
 *	File:		i386/rtclock.c
 *	Purpose:	Routines for handling the machine dependent
 *			real-time clock. This clock is generated by
 *			the Intel 8254 Programmable Interval Timer.
 */

#include <cpus.h>
#include <platforms.h>
#include <mp_v1_1.h>
#include <mach_kdb.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <machine/mach_param.h>	/* HZ */
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <device/dev_hdr.h>	/* for device_t */
#include <device/ds_routines.h>
#include <i386/ipl.h>
#include <i386/pit.h>
#include <i386/pio.h>
#include <i386/misc_protos.h>
#include <i386/rtclock_entries.h>

/*
 * List of real-time clock dependent routines.
 */
struct clock_ops  rtc_ops = {
	rtc_config,	rtc_init,	rtc_gettime,	rtc_settime,
	rtc_getattr,	rtc_setattr,	rtc_maptime,	rtc_setalrm,
};

/* local data declarations */
tvalspec_t		*RtcTime = (tvalspec_t *)0;
tvalspec_t		*RtcAlrm;
clock_res_t		RtcDelt;

/* global data declarations */
struct	{
	mapped_tvalspec_t *mtime;	/* mappable clock time */
#define time		mtime->mtv_time
	tvalspec_t	alarm_time;	/* time of next alarm */
	clock_res_t	new_ires;	/* pending new resolution (nano ) */
	clock_res_t	intr_nsec;	/* interrupt resolution (nano) */
	decl_simple_lock_data(,lock)	/* real-time clock device lock */
} rtclock;
unsigned int clknum;                        /* clks per second */
unsigned int new_clknum;                    /* pending clknum */
unsigned int time_per_clk;                  /* time per clk in ZHZ */
unsigned int clks_per_int;                  /* clks per interrupt */
unsigned int clks_per_int_99;
int          rtc_intr_count;                /* interrupt counter */
int          rtc_intr_hertz;                /* interrupts per HZ */
int          rtc_intr_freq;                 /* interrupt frequency */
int	     rtc_print_lost_tick;	    /* print lost tick */
/*
 *	Macros to lock/unlock real-time clock device.
 */
#define LOCK_RTC(s)			\
	s = splclock();			\
	simple_lock(&rtclock.lock);

#define UNLOCK_RTC(s)			\
	simple_unlock(&rtclock.lock);	\
	splx(s);

/*
 * i8254 control.  ** MONUMENT **
 *
 * The i8254 is a traditional PC device with some arbitrary characteristics.
 * Basically, it is a register that counts at a fixed rate and can be
 * programmed to generate an interrupt every N counts.  The count rate is
 * clknum counts per second (see pit.h), historically 1193167 we believe.
 * Various constants are computed based on this value, and we calculate
 * them at init time for execution efficiency.  To obtain sufficient
 * accuracy, some of the calculation are most easily done in floating
 * point and then converted to int.
 *
 * We want an interrupt every 10 milliseconds, approximately.  The count
 * which will do that is clks_per_int.  However, that many counts is not
 * *exactly* 10 milliseconds; it is a bit more or less depending on
 * roundoff.  The actual time per tick is calculated and saved in
 * rtclock.intr_nsec, and it is that value which is added to the time
 * register on each tick.
 *
 * The i8254 counter can be read between interrupts in order to determine
 * the time more accurately.  The counter counts down from the preset value
 * toward 0, and we have to handle the case where the counter has been
 * reset just before being read and before the interrupt has been serviced.
 * Given a count since the last interrupt, the time since then is given
 * by (count * time_per_clk).  In order to minimize integer truncation,
 * we perform this calculation in an arbitrary unit of time which maintains
 * the maximum precision, i.e. such that one tick is 1.0e9 of these units,
 * or close to the precision of a 32-bit int.  We then divide by this unit
 * (which doesn't lose precision) to get nanoseconds.  For notation
 * purposes, this unit is defined as ZHZ = zanoseconds per nanosecond.
 *
 * This sequence to do all this is in rtc_gettime.  For efficiency, this
 * sequence also needs the value that the counter will have if it has just
 * overflowed, so we precompute that also.  ALSO, certain platforms
 * (specifically the DEC XL5100) have been observed to have problem
 * with latching the counter, and they occasionally (say, one out of
 * 100,000 times) return a bogus value.  Hence, the present code reads
 * the counter twice and checks for a consistent pair of values.
 *
 * Some attributes of the rt clock can be changed, including the
 * interrupt resolution.  We default to the minimum resolution (10 ms),
 * but allow a finer resolution to be requested.  The assumed frequency
 * of the clock can also be set since it appears that the actual
 * frequency of real-world hardware can vary from the nominal by
 * 200 ppm or more.  When the frequency is set, the values above are
 * recomputed and we continue without resetting or changing anything else.
 */
#define RTC_MINRES	(NSEC_PER_SEC / HZ)	/* nsec per tick */
#define	RTC_MAXRES	(RTC_MINRES / 20)	/* nsec per tick */
#define	ZANO		(1000000000)
#define ZHZ             (ZANO / (NSEC_PER_SEC / HZ))
#define READ_8254(val)	{ \
        outb(PITCTL_PORT, PIT_C0);             \
	(val) = inb(PITCTR0_PORT);               \
	(val) |= inb(PITCTR0_PORT) << 8 ; }

/*
 * Calibration delay counts.
 */
unsigned int	delaycount = 10;
unsigned int	microdata = 50;

/*
 * Forward decl.
 */

extern int   measure_delay(int us);
void         rtc_setvals( unsigned int, clock_res_t );

/*
 * Initialize non-zero clock structure values.
 */
void
rtc_setvals(
	unsigned int new_clknum,
	clock_res_t  new_ires
	)
{
    unsigned int timeperclk;
    unsigned int scale0;
    unsigned int scale1;
    unsigned int res;

    clknum = new_clknum;
    rtc_intr_freq = (NSEC_PER_SEC / new_ires);
    rtc_intr_hertz = rtc_intr_freq / HZ;
    clks_per_int = (clknum + (rtc_intr_freq / 2)) / rtc_intr_freq;
    clks_per_int_99 = clks_per_int - clks_per_int/100;

    /*
     * The following calculations are done with scaling integer operations
     * in order that the integer results are accurate to the lsb.
     */
    timeperclk = div_scale(ZANO, clknum, &scale0);	/* 838.105647 nsec */

    time_per_clk = mul_scale(ZHZ, timeperclk, &scale1);	/* 83810 */
    if (scale0 > scale1)
	time_per_clk >>= (scale0 - scale1);
    else if (scale0 < scale1)
	panic("rtc_clock: time_per_clk overflow\n");

    /*
     * Notice that rtclock.intr_nsec is signed ==> use unsigned int res
     */
    res = mul_scale(clks_per_int, timeperclk, &scale1);	/* 10000276 */
    if (scale0 > scale1)
	rtclock.intr_nsec = res >> (scale0 - scale1);
    else
	panic("rtc_clock: rtclock.intr_nsec overflow\n");

    rtc_intr_count = 1;
    RtcDelt = rtclock.intr_nsec/2;
}

/*
 * Configure the real-time clock device. Return success (1)
 * or failure (0).
 */

int
rtc_config(void)
{
	int	RtcFlag;
	int	pic;

#if	NCPUS > 1 && AT386
	mp_disable_preemption();
	if (cpu_number() != master_cpu) {
		mp_enable_preemption();
		return(1);
	}
	mp_enable_preemption();
#endif
	/*
	 * Setup device.
	 */
#if	iPSC386
	pic = 2;
#elif	MP_V1_1
    {
	extern boolean_t mp_v1_1_initialized;
	if (mp_v1_1_initialized)
	    pic = 2;
	else
	    pic = 0;
    }
#else
	pic = 0;
#endif
	take_irq(pic, 0, SPLHI, (intr_t)hardclock);

	/*
	 * We should attempt to test the real-time clock
	 * device here. If it were to fail, we should panic
	 * the system.
	 */
	RtcFlag = /* test device */1;
	printf("realtime clock configured\n");
	return (RtcFlag);
}

/*
 * Initialize the real-time clock device. Return success (1)
 * or failure (0). Since the real-time clock is required to
 * provide canonical mapped time, we allocate a page to keep
 * the clock time value. In addition, various variables used
 * to support the clock are initialized.  Note: the clock is
 * not started until rtclock_reset is called.
 */
int
rtc_init(void)
{
	vm_offset_t	*vp;
#if	NCPUS > 1 && AT386
	mp_disable_preemption();
	if (cpu_number() != master_cpu) {
		mp_enable_preemption();
		return(1);
	}
	mp_enable_preemption();
#endif
	/*
	 * Allocate mapped time page.
	 */
	vp = (vm_offset_t *) &rtclock.mtime;
        if (kmem_alloc_wired(kernel_map, vp, PAGE_SIZE) != KERN_SUCCESS)
                panic("cannot allocate rtclock time page\n");
        bzero((char *)rtclock.mtime, PAGE_SIZE);
	RtcTime = &rtclock.time;
	simple_lock_init(&rtclock.lock, ETAP_NO_TRACE);
	rtc_setvals( CLKNUM, RTC_MINRES );  /* compute constants */
	return (1);
}

static volatile unsigned int     last_ival = 0;

/*
 * Get the clock device time. This routine is responsible
 * for converting the device's machine dependent time value
 * into a canonical tvalspec_t value.
 */
kern_return_t
rtc_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
        tvalspec_t	itime = {0, 0};
	unsigned int	val, val2;
	int		s;

	if (!RtcTime) {
		/* Uninitialized */
		cur_time->tv_nsec = 0;
		cur_time->tv_sec = 0;
		return (KERN_SUCCESS);
	}
#if	(NCPUS == 1 || (MP_V1_1 && 0)) && AT386

	/*
	 * Inhibit interrupts. Determine the incremental
	 * time since the last interrupt. (This could be
	 * done in assembler for a bit more speed).
	 */
	LOCK_RTC(s);
	do {
	    READ_8254(val);                 /* read clock */
	    READ_8254(val2);                /* read clock */
	} while ( val2 > val || val2 < val - 10 );
	if ( val > clks_per_int_99 ) {
	    outb( 0x0a, 0x20 );             /* see if interrupt pending */
	    if ( inb( 0x20 ) & 1 )
		itime.tv_nsec = rtclock.intr_nsec; /* yes, add a tick */
	}
	itime.tv_nsec += ((clks_per_int - val) * time_per_clk) / ZHZ;
	if ( itime.tv_nsec < last_ival ) {
	    if (rtc_print_lost_tick)
		printf( "rtclock: missed clock interrupt.\n" );
	}
	last_ival = itime.tv_nsec;
	cur_time->tv_sec = rtclock.mtime->mtv_sec;
	cur_time->tv_nsec = rtclock.mtime->mtv_nsec;
	UNLOCK_RTC(s);
	ADD_TVALSPEC(cur_time, ((tvalspec_t *)&itime));
#else
	MTS_TO_TS(rtclock.mtime, cur_time);
#endif	/* (NCPUS == 1 || (MP_V1_1 && 0)) && AT386 */
	return (KERN_SUCCESS);
}

/*
 * Get the clock device time when ALL interrupts are already disabled.
 * Same as above except for turning interrupts off and on.
 * This routine is responsible for converting the device's machine dependent
 * time value into a canonical tvalspec_t value.
 */
void
rtc_gettime_interrupts_disabled(
	tvalspec_t	*cur_time)	/* OUT */
{
	tvalspec_t	itime = {0, 0};
	unsigned int	val;

	if (!RtcTime) {
		/* Uninitialized */
		cur_time->tv_nsec = 0;
		cur_time->tv_sec = 0;
		return;
	}
#if	(NCPUS == 1 || (MP_V1_1 && 0)) && AT386

	simple_lock(&rtclock.lock);

	/*
	 * Copy the current time knowing that we cant be interrupted
	 * between the two longwords and so dont need to use MTS_TO_TS
	 */
	READ_8254(val);                     /* read clock */
	if ( val > clks_per_int_99 ) {
	    outb( 0x0a, 0x20 );             /* see if interrupt pending */
	    if ( inb( 0x20 ) & 1 )
		itime.tv_nsec = rtclock.intr_nsec; /* yes, add a tick */
	}
	itime.tv_nsec += ((clks_per_int - val) * time_per_clk) / ZHZ;
	if ( itime.tv_nsec < last_ival ) {
	    if (rtc_print_lost_tick)
		printf( "rtclock: missed clock interrupt.\n" );
	}
	last_ival = itime.tv_nsec;
	cur_time->tv_sec = rtclock.mtime->mtv_sec;
	cur_time->tv_nsec = rtclock.mtime->mtv_nsec;
	ADD_TVALSPEC(cur_time, ((tvalspec_t *)&itime));

	simple_unlock(&rtclock.lock);

#else
	MTS_TO_TS(rtclock.mtime, cur_time);
#endif	/* (NCPUS == 1 || (MP_V1_1 && 0)) && AT386 */
}

/*
 * Set the clock device time. The alarm clock layer has
 * verified the new_time as a legal tvalspec_t value.
 */
kern_return_t
rtc_settime(
	tvalspec_t	*new_time)
{
	spl_t		s;

	LOCK_RTC(s);
	rtclock.mtime->mtv_csec = new_time->tv_sec;
	rtclock.mtime->mtv_nsec = new_time->tv_nsec;
	rtclock.mtime->mtv_sec  = new_time->tv_sec;
	RtcAlrm = 0;
	UNLOCK_RTC(s);

	return (KERN_SUCCESS);
}

/*
 * Get clock device attributes.
 */
kern_return_t
rtc_getattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:	/* >0 res */
#if	(NCPUS == 1 || (MP_V1_1 && 0)) && AT386
		*(clock_res_t *) attr = 1000;
		break;
#endif	/* (NCPUS == 1 || (MP_V1_1 && 0)) && AT386 */
	case CLOCK_MAP_TIME_RES:	/* >0 canonical */
	case CLOCK_ALARM_CURRES:	/* =0 no alarm */
		*(clock_res_t *) attr = rtclock.intr_nsec;
		break;

	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = RTC_MAXRES;
		break;

	case CLOCK_ALARM_MINRES:
		*(clock_res_t *) attr = RTC_MINRES;
		break;
	case CLOCK_BASE_FREQ:
		*(clock_res_t *) attr = clknum;
		break;

	default:
		return (KERN_INVALID_VALUE);
	}
	return (KERN_SUCCESS);
}

/*
 * Set clock device attributes.
 */
kern_return_t
rtc_setattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* IN */
	mach_msg_type_number_t	count)		/* IN */
{
	spl_t		s;
	int		freq;
	int		adj;
	clock_res_t	new_ires;

	if (count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:
	case CLOCK_MAP_TIME_RES:
	case CLOCK_ALARM_MAXRES:
	case CLOCK_ALARM_MINRES:
		return (KERN_FAILURE);

	case CLOCK_ALARM_CURRES:
		new_ires = *(clock_res_t *) attr;

		/*
		 * The new resolution must be within the predetermined
		 * range.  If the desired resolution cannot be achieved
		 * to within 0.1%, an error is returned.
		 */
		if (new_ires < RTC_MAXRES || new_ires > RTC_MINRES)
			return (KERN_INVALID_VALUE);
		freq = (NSEC_PER_SEC / new_ires);
		adj = (((clknum % freq) * new_ires) / clknum);
		if (adj > (new_ires / 1000))
			return (KERN_INVALID_VALUE);
		/*
		 * Record the new alarm resolution which will take effect
		 * on the next HZ aligned clock tick.
		 */
		LOCK_RTC(s);
		if ( freq != rtc_intr_freq ) {
		    rtclock.new_ires = new_ires;
		    new_clknum = clknum;
		}
		UNLOCK_RTC(s);
		return (KERN_SUCCESS);

	    case CLOCK_BASE_FREQ:
		new_clknum = *(clock_res_t *) attr;
		LOCK_RTC(s);
		/* setting new_ires will cause the new settings to
		 * take effect at the next clock tick */
		rtclock.new_ires = NSEC_PER_SEC / rtc_intr_freq;
		UNLOCK_RTC(s);
		return (KERN_SUCCESS);
		

	default:
		return (KERN_INVALID_VALUE);
	}
}

/*
 * Map the clock device time. The real-time clock is required
 * to map a canonical time value. We cheat here, by using the
 * device subsystem to generate a pager for the clock - this
 * should probably be changed at some point.
 */
kern_return_t
rtc_maptime(
	ipc_port_t	*pager)		/* OUT */
{
	device_t	device;
	vm_size_t	size;

	/*
	 * Open the "rtclock" device. The only interface available
	 * to this device is the d_mmap() routine.
	 */
	device = device_lookup("rtclock");
	if (device == DEVICE_NULL)
		return (KERN_FAILURE);

	/*
	 * Note: this code does not touch the device state. This should
	 * be ok even though the device can be opened from user mode via
	 * a device_open() request since both the device and the pager
	 * structures should be protected from deletion by reference
	 * counts and since the dev_pager code doesn't depend upon any
	 * internal device state setup via device_open() other than
	 * that established from device_lookup().
	 */
	size = sizeof(mapped_tvalspec_t);
	return (device_pager_setup(device, VM_PROT_READ, 0, size, pager));
}

/*
 * Set next alarm time for the clock device. This call
 * always resets the time to deliver an alarm for the
 * clock.
 */
void
rtc_setalrm(
	tvalspec_t	*alarm_time)
{
	spl_t		s;

	LOCK_RTC(s);
	rtclock.alarm_time = *alarm_time;
	RtcAlrm = &rtclock.alarm_time;
	UNLOCK_RTC(s);
}



/*
 * Load the count register and start the clock.
 */
#define RTCLOCK_RESET()	{					\
	outb(PITCTL_PORT, PIT_C0|PIT_NDIVMODE|PIT_READMODE);	\
	outb(PITCTR0_PORT, (clks_per_int & 0xff));		\
	outb(PITCTR0_PORT, (clks_per_int >> 8));		\
}

/*
 * Reset the clock device. This causes the realtime clock
 * device to reload its mode and count value (frequency).
 * Note: the CPU should be calibrated
 * before starting the clock for the first time.
 */

void
rtclock_reset(void)
{
	int		s;

#if	NCPUS > 1 && AT386 && !(MP_V1_1 && 0)
	mp_disable_preemption();
	if (cpu_number() != master_cpu) {
		mp_enable_preemption();
		return;
	}
	mp_enable_preemption();
#endif	/* NCPUS > 1 && AT386 && !MP_V1_1 */
 	LOCK_RTC(s);
	RTCLOCK_RESET();
	UNLOCK_RTC(s);
}

/*
 * Map the clock device time. This is part of the "cheat"
 * we use to map the time via the device subsystem. This
 * routine is invoked via a vm_map() call by the user.
 */

vm_offset_t
rtclock_map(
	dev_t		dev,
	vm_offset_t	off,
	int		prot)
{
	vm_offset_t	voff;

#ifdef	lint
	dev++; off++;
#endif	/* lint */

	if (prot & VM_PROT_WRITE)
		return (-1);
	voff = pmap_extract(pmap_kernel(), (vm_offset_t) rtclock.mtime);
	return (i386_btop(voff));
}

/*
 * Real-time clock device interrupt. Called only on the
 * master processor. Updates the clock time and upcalls
 * into the higher level clock code to deliver alarms.
 */
int
rtclock_intr(void)
{
	spl_t		s;
	int		i;
	tvalspec_t	clock_time;
	/*
	 * Update clock time. Do the update so that the macro
	 * MTS_TO_TS() for reading the mapped time works (e.g.
	 * update in order: mtv_csec, mtv_nsec, mtv_sec).
	 */	 
	LOCK_RTC(s);
	i = rtclock.mtime->mtv_nsec + rtclock.intr_nsec;
	if (i < NSEC_PER_SEC)
	    rtclock.mtime->mtv_nsec = i;
	else {
	    rtclock.mtime->mtv_csec++;
	    rtclock.mtime->mtv_nsec = i - NSEC_PER_SEC;
	    rtclock.mtime->mtv_sec++;
	}
	/* note mtime now up to date */
	last_ival = 0;
	/*
	 * Perform alarm clock processing if needed. The time
	 * passed up is incremented by a half-interrupt tick
	 * to trigger alarms closest to their desired times.
	 * The clock_alarm_intr() routine calls rtc_setalrm()
	 * before returning if later alarms are pending.
	 */

	if (RtcAlrm && (RtcAlrm->tv_sec < RtcTime->tv_sec ||
	   	        (RtcAlrm->tv_sec == RtcTime->tv_sec &&
	    		 RtcDelt >= RtcAlrm->tv_nsec - RtcTime->tv_nsec))) {
		clock_time.tv_sec = 0;
		clock_time.tv_nsec = RtcDelt;
		ADD_TVALSPEC (&clock_time, RtcTime);
		RtcAlrm = 0;
		UNLOCK_RTC(s);
		/*
		 * Call clock_alarm_intr() without RTC-lock.
		 * The lock ordering is always CLOCK-lock
		 * before RTC-lock.
		 */
		clock_alarm_intr(REALTIME_CLOCK, &clock_time);
		LOCK_RTC(s);
	}

	/*
	 * On a HZ-tick boundary: return 0 and adjust the clock
	 * alarm resolution (if requested).  Otherwise return a
	 * non-zero value.
	 */
	if ((i = --rtc_intr_count) == 0) {
	    if (rtclock.new_ires) {
		rtc_setvals(
			new_clknum,
			rtclock.new_ires ); /* recompute constants */
		RTCLOCK_RESET();            /* lock clock register */
		rtclock.new_ires = 0;
	    }
	    rtc_intr_count = rtc_intr_hertz;
	}
	UNLOCK_RTC(s);
	return (i);
}

/*
 * measure_delay(microseconds)
 *
 * Measure elapsed time for delay calls
 * Returns microseconds.
 * 
 * Microseconds must not be too large since the counter (short) 
 * will roll over.  Max is about 13 ms.  Values smaller than 1 ms are ok.
 * This uses the assumed frequency of the rt clock which is emperically
 * accurate to only about 200 ppm.
 */

int
measure_delay(
	int us)
{
	unsigned int	lsb, val;

	outb(PITCTL_PORT, PIT_C0|PIT_NDIVMODE|PIT_READMODE);
	outb(PITCTR0_PORT, 0xff);	/* set counter to max value */
	outb(PITCTR0_PORT, 0xff);
	delay(us);
	outb(PITCTL_PORT, PIT_C0);
	lsb = inb(PITCTR0_PORT);
	val = (inb(PITCTR0_PORT) << 8) | lsb;
	val = 0xffff - val;
	val *= 1000000;
	val /= CLKNUM;
	return(val);
}

/*
 * calibrate_delay(void)
 *
 * Adjust delaycount.  Called from startup before clock is started
 * for normal interrupt generation.
 */

void
calibrate_delay(void)
{
	unsigned 	val;
	int 		prev = 0;
	register int	i;

	printf("adjusting delay count: %d", delaycount);
	for (i=0; i<10; i++) {
	  	prev = delaycount;
		/* 
		 * microdata must not be to large since measure_timer
		 * will not return accurate values if the counter (short) 
		 * rolls over
		 */
 		val = measure_delay(microdata);
		delaycount *= microdata;
		delaycount += val-1; 	/* round up to upper us */
		delaycount /= val;
		if (delaycount <= 0)
			delaycount = 1;
		if (delaycount != prev)
			printf(" %d", delaycount);
	}
	printf("\n");
}

#if	MACH_KDB
void
test_delay(void);

void
test_delay(void)
{
  	register i;

	for (i = 0; i < 10; i++)
		printf("%d, %d\n", i, measure_delay(i));
	for (i = 10; i <= 100; i+=10)
		printf("%d, %d\n", i, measure_delay(i));
}
#endif	/* MACH_KDB */
