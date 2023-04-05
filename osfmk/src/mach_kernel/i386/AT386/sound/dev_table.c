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
/*
 * sound/dev_table.c
 *
 * Device call tables.
 *
 * Copyright by Hannu Savolainen 1993
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define _DEV_TABLE_C_
#include <i386/AT386/sound/sound_config.h>

#ifdef CONFIGURE_SOUNDCARD

int snd_find_driver(int type);
long sndtable_init(long mem_start);
int sndtable_get_cardcount(void);
void sound_chconf(int card_type, int ioaddr, int irq, int dma);
struct address_info *sound_getconf(int card_type);

int
snd_find_driver (int type)
{
  int             i, n = sizeof (sound_drivers) / sizeof (struct driver_info);

  for (i = 0; i < (n - 1); i++)
    if (sound_drivers[i].card_type == type)
      return i;

  return -1;			/*
				 * Not found
				 */
}

long
sndtable_init (long mem_start)
{
  int             i, n = sizeof (snd_installed_cards) / sizeof (struct card_info);
  int             drv;

  for (i = 0; i < (n - 1); i++)
    if (snd_installed_cards[i].enabled)
      if ((drv = snd_find_driver (snd_installed_cards[i].card_type)) == -1)
	snd_installed_cards[i].enabled = 0;	/*
						 * Mark as not detected
						 */
      else if (sound_drivers[drv].probe (&snd_installed_cards[i].config))
	{
#ifndef SHORT_BANNERS
	  printk ("snd%d",
		  snd_installed_cards[i].card_type);
#endif

	  mem_start = sound_drivers[drv].attach (mem_start, &snd_installed_cards[i].config);
#ifndef SHORT_BANNERS
	  printk (" at 0x%x irq %d drq %d\n",
		  snd_installed_cards[i].config.io_base,
		  snd_installed_cards[i].config.irq,
		  snd_installed_cards[i].config.dma);
#endif
	}
      else
	snd_installed_cards[i].enabled = 0;	/*
						 * Mark as not detected
						 */
  return mem_start;
}

int sndtable_probe (int unit, struct address_info *hw_config)
{
    int i, n = sizeof (snd_installed_cards) / sizeof (struct card_info);

    if (!unit)
	return TRUE;

    for (i = 0; i < (n - 1); i++)
	if (snd_installed_cards[i].enabled)
	    if (snd_installed_cards[i].card_type == unit) {
		int             drv;

		snd_installed_cards[i].config.io_base = hw_config->io_base;
		snd_installed_cards[i].config.irq = hw_config->irq;
		snd_installed_cards[i].config.dma = hw_config->dma;
		if ((drv = snd_find_driver (snd_installed_cards[i].card_type)) == -1)
		    snd_installed_cards[i].enabled = 0;	/*
							 * Mark as not
							 * detected
							 */
		else if (sound_drivers[drv].probe (hw_config))
		    return 1;
		snd_installed_cards[i].enabled = 0;	/*
							 * Mark as not detected
							 */
		return 0;
	    }

    return FALSE;
}

int
sndtable_init_card (int unit, struct address_info *hw_config)
  {
    int             i, n = sizeof (snd_installed_cards) / sizeof (struct card_info);

    if (!unit)
      {
	if (sndtable_init (0) != 0)
	  panic ("snd: Invalid memory allocation\n");
	return TRUE;
      }
    for (i = 0; i < (n - 1); i++)
      if (snd_installed_cards[i].card_type == unit)
	{
	  int             drv;
	  snd_installed_cards[i].config.io_base = hw_config->io_base;
	  snd_installed_cards[i].config.irq = hw_config->irq;
	  snd_installed_cards[i].config.dma = hw_config->dma;
	  if ((drv = snd_find_driver (snd_installed_cards[i].card_type)) == -1)
	    snd_installed_cards[i].enabled = 0;	/*
						 * Mark as not detected
						 */
	  else {
	      if (sound_drivers[drv].attach (0, hw_config) != 0) {
		  panic ("snd#: Invalid memory allocation\n");
	      }
#ifndef SHORT_BANNER
	  printk (" at 0x%x irq %d dma %d",
		  snd_installed_cards[i].config.io_base,
		  snd_installed_cards[i].config.irq,
		  snd_installed_cards[i].config.dma);
#endif
	      return TRUE;
	  }
	}
    return FALSE;
  }

int
sndtable_get_cardcount (void)
{
  return num_audiodevs + num_mixers + num_synths + num_midis;
}

void
sound_chconf (int card_type, int ioaddr, int irq, int dma)
{
  int             i, n = sizeof (snd_installed_cards) / sizeof (struct card_info);

  int             ptr, j;

  ptr = -1;
  for (j = 0; j < n && ptr == -1; j++)
    if (snd_installed_cards[j].card_type == card_type &&
	!snd_installed_cards[j].enabled)	/*
						 * Not already found
						 */
      ptr = j;

  if (ptr != -1)
    {
      snd_installed_cards[ptr].enabled = 1;
      if (ioaddr)
	snd_installed_cards[ptr].config.io_base = ioaddr;
      if (irq)
	snd_installed_cards[ptr].config.irq = irq;
      if (dma)
	snd_installed_cards[ptr].config.dma = dma;
    }
}

struct address_info *
sound_getconf (int card_type)
{
  int             j, ptr;
  int             n = sizeof (snd_installed_cards) / sizeof (struct card_info);

  ptr = -1;
  for (j = 0; j < n && ptr == -1; j++)
    if (snd_installed_cards[j].card_type == card_type)
      ptr = j;

  if (ptr == -1)
    return (struct address_info *) NULL;

  return &snd_installed_cards[ptr].config;
}

#else

void
sound_setup (char *str, int *ints)
{
}

#endif
