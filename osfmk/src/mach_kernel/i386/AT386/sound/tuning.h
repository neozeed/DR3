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
#ifdef SEQUENCER_C

unsigned short semitone_tuning[24] = 
{
/*   0 */ 10000, 10595, 11225, 11892, 12599, 13348, 14142, 14983, 
/*   8 */ 15874, 16818, 17818, 18877, 20000, 21189, 22449, 23784, 
/*  16 */ 25198, 26697, 28284, 29966, 31748, 33636, 35636, 37755
};

unsigned short cent_tuning[100] =
{
/*   0 */ 10000, 10006, 10012, 10017, 10023, 10029, 10035, 10041, 
/*   8 */ 10046, 10052, 10058, 10064, 10070, 10075, 10081, 10087, 
/*  16 */ 10093, 10099, 10105, 10110, 10116, 10122, 10128, 10134, 
/*  24 */ 10140, 10145, 10151, 10157, 10163, 10169, 10175, 10181, 
/*  32 */ 10187, 10192, 10198, 10204, 10210, 10216, 10222, 10228, 
/*  40 */ 10234, 10240, 10246, 10251, 10257, 10263, 10269, 10275, 
/*  48 */ 10281, 10287, 10293, 10299, 10305, 10311, 10317, 10323, 
/*  56 */ 10329, 10335, 10341, 10347, 10353, 10359, 10365, 10371, 
/*  64 */ 10377, 10383, 10389, 10395, 10401, 10407, 10413, 10419, 
/*  72 */ 10425, 10431, 10437, 10443, 10449, 10455, 10461, 10467, 
/*  80 */ 10473, 10479, 10485, 10491, 10497, 10503, 10509, 10515, 
/*  88 */ 10521, 10528, 10534, 10540, 10546, 10552, 10558, 10564, 
/*  96 */ 10570, 10576, 10582, 10589
};
#else
extern unsigned short semitone_tuning[24];
extern unsigned short cent_tuning[100];
#endif
