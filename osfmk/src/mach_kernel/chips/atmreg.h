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
 * Revision 2.2  93/01/21  12:20:34  danner
 * 	Created.
 * 	[93/01/19  16:29:41  bershad]
 * 
 * Revision 2.3  92/07/09  22:53:14  rvb
 * 	Minimal support for mapping atm fifo's.
 * 	[92/05/12  17:10:10  rvb]
 * 
 * Revision 2.2  92/04/03  12:08:15  rpd
 * 	Created
 * 	[92/03/23            rvb]
 */
/* CMU_EBDHIST */

/*** FORE TCA-100 Turbochannel ATM computer interface ***/

#define RX_COUNT_INTR 	0x0001
#define RX_EOM_INTR 	0x0002
#define RX_TIME_INTR 	0x0004
#define TX_COUNT_INTR 	0x0008
#define RX_CELL_LOST 	0x0010
#define RX_NO_CARRIER 	0x0020
#define CR_RX_ENABLE 	0x0040
#define CR_TX_ENABLE 	0x0080
#define CR_RX_RESET 	0x0100
#define CR_TX_RESET 	0x0200

#define ATM_READ_REG(reg)	((reg) & 0x3ff)	/* 10 bit register mask */


struct atm_device {
  unsigned int prom[64 * 1024 / 4];
  volatile unsigned int sreg;
  volatile unsigned int creg_set;
  volatile unsigned int creg_clr;
  volatile unsigned int creg;
  volatile unsigned int rxtimer;
  unsigned int pad1;
  volatile unsigned int rxtimerv;
  unsigned int pad2;
  volatile unsigned int rxcount;
  unsigned int pad3;
  volatile unsigned int rxthresh;
  unsigned int pad4;
  volatile unsigned int txcount;
  unsigned int pad5;
  volatile unsigned int txthresh;
  unsigned int pad6[64*1024/4 - 15];
  volatile unsigned int rxfifo[14];
  unsigned int pad7[64*1024/4 - 14];
  volatile unsigned int txfifo[14];
  unsigned int pad8[64*1024/4 - 14];
};
/* MUST BE PAGE ALIGNED OR YOU WILL GET KILLED BELOW WITH ATM_INFO */

struct sar_data {
	int header;
	int payload[12];
	int trailer;
};


/*
 * Information for mapped atm device
 */
typedef struct mapped_atm_info {
    volatile unsigned int	interrupt_count;    /* tot interrupts received */
    volatile  unsigned short	saved_status_reg;   /* copy of status reg from last interrupt  */
    unsigned int 	hello_world;
    unsigned		wait_event;
} *mapped_atm_info_t;



#define ATM_DEVICE(p)	(struct atm_device*)(p) 
#define ATM_INFO(p)	(mapped_atm_info_t)( (p) + sizeof(struct atm_device) )
			 
