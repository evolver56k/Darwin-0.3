/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#import <sys/file.h>
#import <bsd/dev/event.h>
#import <bsd/dev/ev_keymap.h>

#import "mac2uni.h"
#import "next2uni.h"
#import "uni2sym.h"

static unsigned char mac2next[ 0x80 ];
static unsigned char mac2nextSet[ 0x80 ];

void makeMac2Next( void )
{
    int key, uni, look;

    for( key = 0; key < 0x80; key++) {

	assert( (key + 0x80) == mac2uni[ key * 2]);

	assert( (key + 0x80) == next2uni[ key * 2]);

	uni = mac2uni[ key * 2 + 1];

        for( look = 0; look < 0x80; look++) {

	    if( uni == next2uni[ look * 2 + 1 ]) {
                mac2next[ key ] = look + 0x80;
		mac2nextSet[ key ] = NX_ASCIISET;
                break;
	    }
	}

	if( mac2next[ key ] != 0)
	    continue;

        for( look = 0; look < sizeof( uni2sym) / 8; look++) {

	    if( uni == uni2sym[ look * 2 ]) {
                mac2next[ key ] = uni2sym[ look * 2 + 1];
		mac2nextSet[ key ] = NX_SYMBOLSET;
                break;
	    }
	}

	if( mac2next[ key ] == 0) {
	    printf("Mac Roman 0x%x = unicode 0x%x not found\n", key + 0x80, uni);
	}
    }
}

/*----------------------------KCHR ¥ ASCII Mapping (software)-----------------*/
#if 0
type 'KCHR' {
        integer;                                      /* Version                */
        wide array [$100] {                           /* Indexes                */
            byte;
        };
        integer = $$CountOf(TableArray);
        array TableArray {
            wide array [$80] {                        /* ASCII characters        */
                char;
            };
        };
        integer = $$CountOf(DeadArray);
        array DeadArray {
            byte;                                     /* Table number            */
            byte;                                     /* Virtual keycode        */
            integer = $$CountOf(CompletorArray);
            wide array CompletorArray {
                char;                                 /* Completor char        */
                char;                                 /* Substituting char    */
            };
            char;                                     /* No match char        */
            char;                                     /* 16-bits for the times
                                                         when 8 isn't enough    */
        };
};
#endif

int searchDead( int tableNum, int keycode,
	unsigned char *	dead, int ndead )
{
    int	thisCount;

    while( ndead--) {

        thisCount = dead[ 3 ];
        if( (tableNum == dead[ 0 ]) && (keycode == dead[ 1 ])) {

                dead += (thisCount * 2) + 2 + 2 + 1;
		printf("Dead key for %d, %02x = %02x\n", tableNum, keycode, *dead);
		return( *dead);
	}

	dead += (thisCount * 2) + 2 + 2 + 2;

    }
    return( 0);
}


#define NKEYCODES	0x7b
#define NFKEYS		28

#define NOTPRESENTCHAR	0xe0
#define NOTPRESENTSET	NX_SYMBOLSET
//#define NOTPRESENTCHAR	0
//#define NOTPRESENTSET		0

unsigned char *
doit( unsigned short * kchr, unsigned char * map, int doEuro)
{
    unsigned char *	indexes;
    unsigned char *	table;
    unsigned char *	dead;
    int			vers, ntables, ndead, isDead;
    int			nextChr, nextSet, key, mac, switched;
    int			mods, active;
    int			i, j;

    static unsigned char nextToMacModBit[] = { 2, 1, 4, 3, 0 };
    static unsigned char nextToMacMod[ 32 ];

    static unsigned int onekey[ 32 ];

    printf("KCHR vers = %x", kchr[0]);
    ntables = kchr[ 0x81 ];
    printf(", table count = %x", ntables);

    assert( ntables > 0);

    indexes = (unsigned char *) &kchr[ 1 ];

    dead = (unsigned char *) &kchr[ 0x82 ];
    dead += 0x80 * ntables;

    dead++;
    ndead = *dead;
    dead++;
    
    printf(", dead count = %x\n", ndead);

    bzero( nextToMacMod, sizeof( nextToMacMod));
    bzero( onekey, sizeof( onekey));

    for( i = 0; i <= NX_MODIFIERKEY_COMMAND; i++)
	for( j = (1 << i); j < 32; j++)
	    if( j & (1 << i))
                nextToMacMod[ j ] |= (1 << nextToMacModBit[ i ]);


    for( key = 0; key <= (NKEYCODES - NFKEYS); key += 1) {

	switched = 0;

        for( mods = 0; mods < 32; mods++) {
//        printf("Next mod %x = Mac mod %x\n", mods, nextToMacMod[ mods ]);

            table = (unsigned char *) &kchr[ 0x82 ];
            table += 0x80 * indexes[ nextToMacMod[ mods ] ];

            mac = table[ key ];
            nextChr = 0;
            nextSet = 0xff;

            // dead key?
	    isDead = (mac == 0);
            if( isDead) {
		mac = searchDead( indexes[ nextToMacMod[ mods ] ], key,
				dead, ndead );
	    }

	    if( (key >= 0x3b) && (key <= 0x3e)) {
		// cursor key
		static const unsigned char macCursor2Next[] = { 0xac, 0xae, 0xaf, 0xad };
                nextChr = macCursor2Next[ key - 0x3b ];
                nextSet = NX_SYMBOLSET;

	    } else if( key == 0x33) {
		// delete (backspace)
                nextSet = NX_ASCIISET;
		if( mods & (1 << NX_MODIFIERKEY_SHIFT))
		    nextChr = 0x08;
		else
		    nextChr = 0x7f;

	    } else if( key == 0x30) {
		// tab is wrong for shift case
                nextSet = NX_ASCIISET;
		if( mods & (1 << NX_MODIFIERKEY_SHIFT))
		    nextChr = 0x19;
		else
		    nextChr = 0x09;

	    } else if( key == 0x13 && (mods & (1 << NX_MODIFIERKEY_CONTROL))) {
		// 2 is wrong for control case
                nextSet = NX_ASCIISET;
                nextChr = 0x00;

	    } else if( key == 0x16 && (mods & (1 << NX_MODIFIERKEY_CONTROL))) {
		// 6 is wrong for control case
                nextSet = NX_ASCIISET;
                nextChr = 0x1e;

	    } else if( doEuro && (key == 0x15) && 
		(((1 << NX_MODIFIERKEY_ALTERNATE) | (1 << NX_MODIFIERKEY_SHIFT)) ==
                (mods & ((1 << NX_MODIFIERKEY_ALTERNATE) | (1 << NX_MODIFIERKEY_SHIFT))))
		) {
		// make option-shift-4 euro symbol
                nextSet = NX_SYMBOLSET;
                nextChr = 0xa0;

	    } else if( key == 0x31) {
		// space needs option & control cases
                nextSet = NX_ASCIISET;
                nextChr = 0x20;
		if( mods & (1 << NX_MODIFIERKEY_CONTROL))
		    nextChr = 0x00;
		else if( mods & (1 << NX_MODIFIERKEY_ALTERNATE))
		    nextChr = 0x80;

	    } else if( mac == 0) {

                nextChr = NOTPRESENTCHAR;
                nextSet = NOTPRESENTSET;
//            } else if( mac < 0x20) {
//                   printf("Key 0x%x, Mac Roman 0x%x < 0x20\n", key, mac);

            } else if( mac < 0x80) {

                nextSet = NX_ASCIISET;
                nextChr = mac;
#if 1
		if( switched &&
		(mods == ((1 << NX_MODIFIERKEY_CONTROL) | (1 << NX_MODIFIERKEY_ALTERNATE)))) {
		    nextChr = switched;
		    switched = 0;
		}

		// fix up some of the accents
		if( isDead) {
		    switched = mac;
                    if( mac == 0x5e)
                        nextChr = 0xc3;
                    else if( mac == 0x60)
                        nextChr = 0xc1;
                    else if( mac == 0x7e)
                        nextChr = 0xc4;
		    else
			switched = 0;
		}
#endif
            } else {

                nextChr = mac2next[ mac - 0x80];
                nextSet = mac2nextSet[ mac - 0x80];
                if( nextChr == 0) {
                    printf("Key 0x%x, mod %x, Mac Roman 0x%x not mappable\n", key, mods, mac);
                    // not present char
                    nextChr = NOTPRESENTCHAR;
                    nextSet = NOTPRESENTSET;
                }
            }

	    onekey[ mods ] = nextChr + (nextSet << 16);

//            printf("Mod mask %02x = %02x, %d\n", mods, nextChr, nextSet);

        } // each mod

	active = 0;
        for( i = 0; i < NX_MODIFIERKEY_COMMAND; i++) {

            for( mods = 0; mods < 16; mods++) {

                if( onekey[ mods ] != onekey[ mods | (1 << i)]) {

                    active |= ( 1 << i );
                    break;
                }
            }
	}
#if 1
	if( (active & ((1 << NX_MODIFIERKEY_SHIFT) | (1 << NX_MODIFIERKEY_ALPHALOCK)))
            == ((1 << NX_MODIFIERKEY_SHIFT) | (1 << NX_MODIFIERKEY_ALPHALOCK))) {

            active &= ~((1 << NX_MODIFIERKEY_SHIFT) | (1 << NX_MODIFIERKEY_ALPHALOCK));

            for( mods = 0; mods < 16; mods += 4) {

                if( onekey[ mods + 1 ] != onekey[ mods + 2])
                    active |= (1 << NX_MODIFIERKEY_SHIFT);
                else
                    active |= (1 << NX_MODIFIERKEY_ALPHALOCK);

		if( onekey[ mods + 2 ] != onekey[ mods + 3]) {
		    // shift => alphalock on next
		    printf("Trouble(shift/caps) ");
                    onekey[mods + 3] = onekey[mods + 2];
		}
            }
	}
#endif

	if( (active == 0) 
	&& (onekey[0] == (NOTPRESENTCHAR + (NOTPRESENTSET << 16)) ))  {

	    active = 0xff;
            *map++ = active;
            printf("Inactive %02x = %02x\n", key, active);
	    continue;
	}

	printf("Active %02x = %02x :: ", key, active);
	*map++ = active;

        for( mods = 0; mods < 16; mods++) {

	    if( mods & ~active)
		continue;

	    *map++ = onekey[ mods ] >> 16;
	    *map++ = onekey[ mods ] & 0xff;

	    printf("%02x = %d,%02x, ", mods, *(map - 2), *(map - 1));

	}
	printf("\n");

    } // each keycode

    return( map);
}

unsigned char *
startMap( unsigned char * map)
{
    static unsigned char mapBegin[] = {
        0x00,0x00,
        0x06,
//        0x00,0x01,0x39,
        0x01,0x02,0x38,0x7b,
        0x02,0x02,0x36,0x7d,
        0x03,0x02,0x3a,0x7c,
        0x04,0x01,0x37,

        0x05,
        0x15,0x52,0x41,0x4c,0x53,0x54,0x55,0x45,0x58,0x57,0x56,0x5b,0x5c,
        0x43,0x4b,0x51,0x3b,0x3d,0x3e,0x3c,0x4e,0x59,

        0x06,0x01,0x72,

	NKEYCODES,

    };

    bcopy( mapBegin, map, sizeof( mapBegin));

    return( map + sizeof( mapBegin));
}

unsigned char *
endMap( unsigned char * map)
{
    static unsigned char mapEnd[] = { 
	// 0x60 -> 0x7a
	//  F1-F15, ins, del, page up/down, home,end
        0x00,0xfe,0x24,
	0x00,0xfe,0x25,
	0x00,0xfe,0x26,
	0x00,0xfe,0x22,
	0x00,0xfe,0x27,
	0x00,0xfe,0x28,
	0xff,
	0x00,0xfe,0x2a,
	0xff,
        0x00,0xfe,0x32,
	0xff,
	0x00,0xfe,0x33,
	0xff,
	0x00,0xfe,0x29,
	0xff,
	0x00,0xfe,0x2b,
	// 0x70
	0xff,
        0x00,0xfe,0x34,
	0xff,
	0x00,0xfe,0x2e,
	0x00,0xfe,0x30,
	0x00,0xfe,0x2d,
	0x00,0xfe,0x23,
        0x00,0xfe,0x2f,
	0x00,0xfe,0x21,
	0x00,0xfe,0x31,
	0x00,0xfe,0x20,

	// sequences
	0x00,
	// special keys
        0x05,
        0x04,0x39,0x05,0x72,0x06,0x7f,0x07,0x3e,0x08,0x3d
    };

    bcopy( mapEnd, map, sizeof( mapEnd));

    return( map + sizeof( mapEnd));
}

void fail( char * err)
{
    printf("%s\n",err);
    exit(1);
}

int
main( int argc, char * argv[] )
{
    int infd, outfd;
    int size = 8 * 1024;
    int writeSize;

    unsigned short * kchr = (unsigned short *)malloc( size );
    unsigned char * map = (unsigned char *)malloc( size );
    unsigned char * orig = (unsigned char *)malloc( 50000 );
    char newName[ 64 ];
    unsigned int * header;
    char * file;
    unsigned char * end;
    int		doEuro = 0;

    if( argc < 3) {
	printf("%s KCHR-file euro [input-file handlerID]\n", argv[0] );
	exit(0);
    }

    assert( kchr && map && orig);

    doEuro = argv[2][0] == 'y';

    header = (unsigned int *) map;
    map += 0x10;

    assert( (infd = open(argv[1], O_RDONLY, 0)) >= 0);

    size = read(infd, kchr, size);
    close( infd);

    assert( size && (size != 8 * 1024));

    makeMac2Next();

    end = endMap( doit( kchr, startMap( map), doEuro ));

    header [ 0 ] = 0x4b594d31;
    header [ 1 ] = 2;
    header [ 2 ] = 21;
    header [ 3 ] = end - map;


    if ( (argc > 4) && (infd = open(argv[3], O_RDONLY, 0)) >= 0)
    {
        unsigned char * dataPtr, *writePtr;
	int data, length, interface, handler;

#define READINT( data, p)	\
    data  = (*p++) << 24;	\
    data |= (*p++) << 16;	\
    data |= (*p++) << 8;	\
    data |= (*p++) << 0;

        header [ 2 ] = strtol( argv[4], 0, 0);

        file = argv[3] + strlen( argv[3]);
        while( (file > argv[3]) && ((*(file-1)) != '/'))
            file--;

	sprintf( newName, "new/%s", file);
	printf("\nFrom %s to %s\n", argv[3], newName);

        assert( (outfd = open(newName, O_CREAT | O_WRONLY, 0)) >= 0);

        size = read(infd, orig, 50000);
        close(infd);

        dataPtr = orig;
        READINT(data,dataPtr);
        if( data != 0x4b594d31) {
            printf( "Type : %x :", data );
            fail( "Bad map type");
        }

        assert( 4 == write(outfd, orig, 4));

        while( 1) {

            if( ((int)dataPtr - (int)orig) == size) {

                printf("Adding Interface %d, Handler %d\n", header[1], header[2]);
                writeSize = header[3] + 12;
                assert( writeSize == write(outfd, &header[1], writeSize));
                break;
            }

	    assert( ((int)dataPtr - (int)orig) < (size - 12));

            READINT(interface, dataPtr);
            READINT(handler, dataPtr);
            READINT(length, dataPtr);

            printf("Output Interface %d, Handler %d", interface, handler );

            if( interface != 2) {
		printf("\n");
                writeSize = length + 12;
                assert( writeSize == write(outfd, dataPtr - 12, writeSize));
            } else {
		printf(" skipped\n");
            }

            dataPtr += length;
        }
        close( outfd);

    } else {
        file = argv[1] + strlen( argv[1]);
        while( (file > argv[1]) && ((*(file-1)) != '/'))
            file--;
	sprintf( newName, "new/%s.keymapping", file);
        printf("Making %s\n", newName);
        assert( (outfd = open(newName, O_CREAT | O_WRONLY, 0)) >= 0);
        writeSize = header[3] + 16;
        assert( writeSize == write(outfd, &header[0], writeSize));
    }

    printf("Done\n");
}

