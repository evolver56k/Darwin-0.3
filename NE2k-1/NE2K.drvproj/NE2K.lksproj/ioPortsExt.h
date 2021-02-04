/*
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * Primatives for IO port access.
 *
 * HISTORY
 *
 * 16Feb93 David E. Bohman at NeXT
 *	Created.
 */

#ifndef _DRIVERKIT_I386_IOPORTS_EXT_
#define _DRIVERKIT_I386_IOPORTS_EXT_

#import <driverkit/i386/driverTypes.h>

static __inline__
void
insb(
    IOEISAPortAddress	port,
    void		*addr,
    int			cnt
)
{
        __asm __volatile("cld; rep; insb"
                         : : "d" (port), "D" (addr), "c" (cnt)
                         : "di", "cx", "memory");
}
 
static __inline__
void
insw(
    IOEISAPortAddress	port,
    void		*addr,
    int			cnt
)
{
        __asm __volatile("cld; rep; insw"
                         : : "d" (port), "D" (addr), "c" (cnt)
                         : "di", "cx", "memory");
}

static __inline__
void
insl(
     IOEISAPortAddress	port,
     void		*addr,
     int		cnt
 )
{
        __asm __volatile("cld; rep; insl"
                         : : "d" (port), "D" (addr), "c" (cnt)
                         : "di", "cx", "memory");
}


static __inline__
void
outsb(
     IOEISAPortAddress	port,
     void		*addr,
     int		cnt
 )
{
        __asm __volatile("cld; rep; outsb"
                         : : "d" (port), "S" (addr), "c" (cnt)
                         : "si", "cx");
}

static __inline__
void
outsw(
     IOEISAPortAddress	port,
     void		*addr,
     int		cnt
 )
{
        __asm __volatile("cld; rep; outsw"
                         : : "d" (port), "S" (addr), "c" (cnt)
                         : "si", "cx");
}

static __inline__
void
outsl(
     IOEISAPortAddress	port,
     void		*addr,
     int		cnt
 )
{
        __asm __volatile("cld; rep; outsl"
                         : : "d" (port), "S" (addr), "c" (cnt)
                         : "si", "cx");
}
#endif	_DRIVERKIT_I386_IOPORTS_EXT_
