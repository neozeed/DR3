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
 * Define a service to map from a kernel-generated port name
 * to server-defined "type" and "value" data to be associated
 * with the port.
 */

#ifndef PORT_OBJ_H
#define PORT_OBJ_H

#include <mach/port.h>

struct port_obj_tentry {
	void *pos_value;
	int pos_type;
};

extern void port_obj_init(int);
extern struct port_obj_tentry *port_obj_table;
extern int port_obj_table_size;

#ifndef PORT_OBJ_ASSERT

#define port_set_obj_value_type(pname, value, type)	\
do {							\
	int ndx;					\
							\
	if (!port_obj_table)				\
		port_obj_init(port_obj_table_size);	\
	ndx = MACH_PORT_INDEX(pname);			\
	port_obj_table[ndx].pos_value = (value);	\
	port_obj_table[ndx].pos_type = (type);		\
} while (0)

#define port_get_obj_value(pname)			\
	(port_obj_table[MACH_PORT_INDEX(pname)].pos_value)

#define port_get_obj_type(pname)			\
	(port_obj_table[MACH_PORT_INDEX(pname)].pos_type)

#else	/* PORT_OBJ_ASSERT */

#define port_set_obj_value_type(pname, value, type)	\
do {							\
	int ndx;					\
							\
	if (!port_obj_table)				\
		port_obj_init(port_obj_table_size);	\
	ndx = MACH_PORT_INDEX(pname);			\
	assert(ndx > 0);				\
	assert(ndx < port_obj_table_size);		\
	port_obj_table[ndx].pos_value = (value);	\
	port_obj_table[ndx].pos_type = (type);		\
} while (0)

#define port_get_obj_value(pname)				\
	((MACH_PORT_INDEX(pname) < (unsigned)port_obj_table_size) ?	\
	port_obj_table[MACH_PORT_INDEX(pname)].pos_value :	\
	(panic("port_get_obj_value: index too big"), (void *)-1))

#define port_get_obj_type(pname)				\
	((MACH_PORT_INDEX(pname) < (unsigned)port_obj_table_size) ?	\
	port_obj_table[MACH_PORT_INDEX(pname)].pos_type :	\
	(panic("port_get_obj_type: index too big"), -1))

#endif	/* PORT_OBJ_ASSERT */

#endif	/* PORT_OBJ_H */
