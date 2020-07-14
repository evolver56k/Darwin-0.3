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
//
// NVRAM editor  -  Set (or print) OpenFirmware NVRAM variables.
//
//
// Derived from code written by Alan Mimms under the MacOS for Copland and Rhapsody.
// Ported by Eryk Vershen <eryk@apple.com> in July 1997.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

// Defines
#define DUMP 0
#define NVfetchB(A)	ReadNVRAMByte(A)
#define NVstoreB(A,B)	WriteNVRAMByte ((A), (B))
#define FetchB(BP)	(*(BP))
#define FetchW(WP)	(*(WP))
#define FetchL(LP)	(*(LP))
#define StoreB(BP,B)	(*(BP) = (B))
#define StoreW(WP,W)	(*(WP) = (W))
#define StoreL(LP,L)	(*(LP) = (L))

// Constants
enum {
    kMaxImageSize = 0x2000,
    kNVRAMSize = 0x2000,
    kNVRAMPageSize = 0x100,
    kMaxStringSize = 0x800,
    kMaxNameSize = 0x100,
    kOpenFirmwareMagic = 0x1275
};

enum {
    kFirstColumn,
    kScanComment,
    kFindName,
    kCollectName,
    kFindValue,
    kCollectValue,
    kContinueValue,
    kSetenv
};

#if DUMP
char *state_names[] = {
    "kFirstColumn",
    "kScanComment",
    "kFindName",
    "kCollectName",
    "kFindValue",
    "kCollectValue",
    "kContinueValue",
    "kSetenv"
};
#endif

#define OF_env_INVALID_NAME	-1
#define OF_env_ILLEGAL_VALUE	-2
#define OF_env_BAD_CHECKSUM	-3
#define OF_end_NO_ROOM		-4

enum {
    kOFboolean = 1,
    kOFlong = 2,
    kOFstring = 3
};

// Typedefs
typedef char* charP;
typedef unsigned char	byte, *byteP;
typedef unsigned short  half, *halfP;
typedef unsigned long	word, *wordP;

typedef struct {
    half	of_magic;
    byte	of_version;
    byte	of_pages;
    half	of_checksum;
    half	of_here;
    half	of_top;
    half	of_next;
    word	of_flags;
} of_hdr, *of_hdrP;

typedef struct	{
    charP	name;
    int		type;
    int		offset;
} dict_entry;

typedef struct {
    charP	name;
    charP	value;
} patch_entry;

struct NVRAMPart {
  byte signature;
  byte checksum;
  half length;
  byte name[12];
};
typedef struct NVRAMPart NVRAMPart;

struct NewDict {
  struct NewDict *next;
  charP          name;
  charP          value;
  int            length;
};
typedef struct NewDict NewDict, *NewDictPtr;

// prototypes
int OpenNewDict(charP buffer);
int CloseNewDict(charP buffer);

// Variables
char *program;
extern int errno;
extern int sys_nerr;
extern const char * const sys_errlist[];

int debug;
int ignore_checksum = 0;

static int of_fd;
static int ofBase;
static int ofSize;

static int wasChanged;
static int isNewWorld;
static NewDictPtr newDict, newDictLast;

static byte cur_image[kMaxImageSize];
static byte new_image[kMaxImageSize];

static const dict_entry dict_table[] =	{
    { "little-endian?",	kOFboolean,	0 },	// bit number (from left) in of_flags
    { "real-mode?",	kOFboolean,	1 },
    { "auto-boot?",	kOFboolean,	2 },
    { "diag-switch?",	kOFboolean,	3 },
    { "fcode-debug?",	kOFboolean,	4 },
    { "oem-banner?",	kOFboolean,	5 },
    { "oem-logo?",	kOFboolean,	6 },
    { "use-nvramrc?",	kOFboolean,	7 },
    { "real-base",	kOFlong,	16 },	// offset of long value
    { "real-size",	kOFlong,	20 },
    { "virt-base",	kOFlong,	24 },
    { "virt-size",	kOFlong,	28 },
    { "load-base",	kOFlong,	32 },
    { "pci-probe-list",	kOFlong,	36 },
    { "screen-#columns",kOFlong,	40 },
    { "screen-#rows",	kOFlong,	44 },
    { "selftest-#megs",	kOFlong,	48 },
    { "boot-device",	kOFstring,	52 },	// offset of <offset,length> pair
    { "boot-file",	kOFstring,	56 },
    { "diag-device",	kOFstring,	60 },
    { "diag-file",	kOFstring,	64 },
    { "input-device",	kOFstring,	68 },
    { "output-device",	kOFstring,	72 },
    { "oem-banner",	kOFstring,	76 },
    { "oem-logo",	kOFstring,	80 },
    { "nvramrc",	kOFstring,	84 },
    { "boot-command",	kOFstring,	88 },
    { 0,		0,		0}	// mark the end of the table
};

static const patch_entry patch_table[] = {
    { "auto-boot?",	"true" },
    { "use-nvramrc?",	"true" },
    { "boot-command",	"0 bootr" },
    { "boot-device",	"scsi/sd@3:0" },
    { "boot-file",	"scsi/sd@3:7,mach_kernel" },
    { "load-base",	"600000" },
    { "nvramrc",
	    ": $C $call-method ;\r"
	    ": $D find-device ;\r"
	    ": $E device-end ;\r"
	    ": $x execute ;\r"
	    ": $F $D \" open\" $find drop ;\r"
	    ": $p 0 to my-self property ;\r"
	    ": $a \" /chosen\" $D $p $E ;\r"
	    ": $R BRpatch ; : $L BLpatch ;\r"
	    ": '& get-token drop ;\r"
	    ": >& dup @ 6 << 6 >>a -4 and + ;\r"
	    ": & na+ >& ;\r"
	    "6ED '& $x\r"
	    "0 value mi\r"
	    ": mmr \" map-range\" mi if my-self $C else $call-parent then ;\r"
	    "89B '& ' mmr $R\r"
	    ": mcm -1 to mi $C 0 to mi ;\r"
	    "8CB '& 1E na+ ' mcm $L\r"
	    ": maa -1 to mi 1D swap ;\r"
	    "8C9 '& 5 na+ ' maa $L\r"
	    "8C9 '& 134 + ' 1 $L\r"
	    "8CD '& 184 + dup 14 + >& $R\r"
	    "8C6 '& 7C + ' u< $L\r"
	    "0 value yn\r"
	    ": y yn 0= if dup @ to yn then ;\r"
	    "8CB '& ' y $R\r"
	    "' y 28 + 8CB '& 8 + $R\r"
	    ": z yn ?dup if over ! 0 to yn then ;\r"
	    "8CC '& ' z $R\r"
	    "' z 2C + 8CC '& 8 + $R\r"
	    "@startvec BC + @ 40820014 over 88 + code! 41820010 swap E0 + code!\r"
	    "0 @startvec 5C + @ 1D8 + code!\r"
	    "dev /packages/mac-parts\r"
	    "400000 ' load 14 + code!\r"
	    ": m1 400000 do-unmap ;\r"
	    "' load 8 + ' m1 $L\r"
	    ": &r1 4+ dup 8000 alloc-mem 7F00 + swap ! 4+ F8 ;\r"
	    "' load 2AC - ' &r1 $L\r"
	    "$E\r"
	    "4180FFF0 ' msr! 44 + code!\r"
	    "dev /packages/xcoff-loader\r"
	    ": p&+ ['] open 600 - + ;\r"
	    ": p1 { _a _s } _a -1000 and _a _s + over - FFF ;\r"
	    "60000000 dup 8 p&+ code! C p&+ code!\r"
	    "18 p&+ ' p1 $L\r"
	    "$E\r"
	    "\" enet\" $F dup\r"
	    "1D8 - dup 24 + ['] or $L $x\r"
	    "248 - @ 6 encode-bytes 2dup\r"
	    "\" local-mac-address\" $p $E\r"
	    "\" mac-address\" $a\r"
	    ": bootr 0d word count\r"
	    "encode-string\r"
	    "\" machargs\" $a\r"
	    "\" kbd\" $D \" get-key-map\" $find drop $x $E drop\r"
	    "4 + @ 40 and xor\r"
	    "if bye\r"
	    "else cr boot then\r"
	    ";\r"
	    ": V&\r"
	    "dup dup @ -F0001 and swap code!\r"
	    "14 +\r"
	    ";\r"
	    "' CALLBACK 158 - V& V& V&\r"
	    ": &SI\r"
	    "\" scsi-int\" open-dev ?dup if\r"
	    "\" open\" 2 pick 4+ @ find-method drop\r"
	    "dup 2c + ['] 2 $l 848 -\r"
	    "dup 8 + dup 1C + $R\r"
	    "dup 88 + dup 4+ $R\r"
	    "$x\r"
	    "\" close\" rot $C\r"
	    "else\r"
	    "\" devalias scsi-int scsi\" eval\r"
	    "then\r"
	    ";\r"
	    "&SI\r"
	    "\" /packages/obp-tftp\" $F\r"
	    "718 - 94 + ['] drop $L\r"
	    "$E\r" },
    { 0,		0 }
};

/*
 * error ( format_string, item )
 *
 *	Print a message on standard error.
 */
void
error(value, fmt, x1, x2)
int value;
char *fmt;
{
    fprintf(stderr, "%s: ", program);
    fprintf(stderr, fmt, x1, x2);

    if (value > 0 && value < sys_nerr) {
	fprintf(stderr, "  (%s)\n", sys_errlist[value]);
    } else {
	fprintf(stderr, "\n");
    }
}

/*
 * fatal ( value, format_string, item )
 *
 *	Print a message on standard error and exit with value.
 *	Values in the range of system error numbers will add
 *	the perror(3) message.
 */
void
fatal(value, fmt, x1, x2)
int value;
char *fmt;
{
    fprintf(stderr, "%s: ", program);
    fprintf(stderr, fmt, x1, x2);

    if (value > 0 && value < sys_nerr) {
	fprintf(stderr, "  (%s)\n", sys_errlist[value]);
    } else {
	fprintf(stderr, "\n");
    }

    exit(value);
}

/*
 * usage ( kind )
 *
 *	Bad usage is a fatal error.
 */
void
usage(kind)
char *kind;
{
    error(-1, "(usage: %s)", kind);
    printf("%s [-p] [-i] [-f filename] name[=value] ...\n", program);
    printf("\t-p         print all open firmware variables\n");
    printf("\t-i         use this program's built-in settings\n");
    printf("\t-f         set open firmware variables from a text file\n");
    printf("\tname=value set named variable\n");
    printf("\tname       print variable\n");
    printf("Note that arguments and options are executed in order.\n");
    exit(1);
}

#if DUMP
//
// Prints the current image of OF NVRAM to stdout
// in hex and ascii.
//
static void
dump_image()
{
    int i, j;
    char c;

    printf( "image:\n" );
    for (i=0; i < ofSize; i++) {
	if( (i & 0xF) == 0 )
	    printf( "%04X: ", i );
	printf( "%02X", cur_image[i] );
	if( (i & 0xF) == 0xF ) {
	    printf( " |" );
	    for( j=i-16; j<i; j++ )
		printf( "%c", isprint(c=cur_image[j+1]) ? c : '_' );
	    printf( "|\n" );
	}
    }
}
#endif

//
// Read bytes the hard way.
//
static byte
ReadNVRAMByte(unsigned long address)
{
    byte buf;
    off_t old_off;

    if ((old_off = lseek(of_fd, (off_t)address, 0)) < 0) {
	fatal(errno, "Couldn't seek to %x", address);
    }
    if (read(of_fd, &buf, 1) < 0) {
	fatal(errno, "Couldn't read from nvram");
    }
//printf("(0x%x) -> 0x%x\n", address, buf);
    return buf;
}

//
// Write bytes one ... at ... a ... time
//
static void
WriteNVRAMByte(unsigned long address, byte data)
{
    off_t old_off;

    if ((old_off = lseek(of_fd, (off_t)address, 0)) < 0) {
	fatal(errno, "Couldn't seek to %x", address);
    }
    if (write(of_fd, &data, 1) < 0) {
	fatal(errno, "Couldn't write to nvram");
    }
//printf("(0x%x) <- 0x%x\n", address, data);
}		


//
// Why two checksum routines? OpenFirmare's method of creating the
// checksum value will sometimes not actually sum to zero!
//
// A 32-bit data sum (with 16-bit checksum field set to zero) of
// 0xfdffeb is an example. OF calculates the checksum field
// to be ~(0xfd + 0xffeb) = 0xff17. Running the same checksum
// algorithm with the new checksum field gives a 32-bit sum of
// 0xfeff02, and a checksum of ~(0xfe + 0xff02) = 0xffff, 
// ie. non-zero and a failure.
//
// The VerifyChecksum() routine will check for this and succeed.
// The (Alan Mimms version) checksum() routine will create a new 
// checksum field that passes both the OF and its own algorithm.
//

//
// Every little bit counts
//
static word
VerifyChecksum( byteP image )
{
    word sum = 0;
    word ofck;
    unsigned int i, cnt;
    byte b_data, i_sum, c_sum;

    if (isNewWorld) {
      c_sum = 0;
      for (cnt = 0; cnt < 0x10; cnt++) {
	if (cnt == 1) continue; // don't include the checksum.
	
	b_data = image[cnt];
	i_sum = c_sum + b_data;
	if (i_sum < c_sum) i_sum++;
	c_sum = i_sum;
      }
      ofck = c_sum - image[1];

    } else {

      for( i=0; i<ofSize; i +=2 )
        sum += FetchW( (halfP)&image[i] );
      
      ofck = (sum >> 16) + (sum & 0xffff);
      if( ofck == 0x10000)
	ofck--;		// Hit the bug, make it OK
      ofck = (ofck ^ 0xffff) & 0xffff;
    }
    
    return ofck;
}

//
// Every little bit helps
//
static word
checksum( byteP image )
{
    word CKS=0;
    int i;

    for( i=0; i<ofSize; i +=2 )
       CKS += FetchW( (halfP)&image[i] );
    return CKS % 0xFFFF;
}


//
// Open the device, find the OF area and make a copy of the data
// so we don't mess with the live stuff.
//
static void 
openHardwareNVRAM ()
{
    int       i, cnt;
    NVRAMPart part;
    byte      *partPtr = (byte *)&part;

    if ((of_fd = open("/dev/nvram", O_RDWR, 0)) < 0) {
	fatal(errno, "Couldn't open NVRAM device");
    }

    // Assume for a moment that this is a New World machine...
    ofBase = 0;
    ofSize = 0;			// Assume failure
    while (ofBase < kNVRAMSize) {
      for (cnt = 0; cnt < 0x10; cnt++) partPtr[cnt] = NVfetchB(ofBase + cnt);
      // I don't think zero is a valid signature.
      if (part.signature == 0x00) break;
      
      if ((part.signature == 0x70) && (strcmp(part.name, "common") == 0)) {
	// Found it.
	isNewWorld = 1;
	ofSize = 0x10 * part.length;
	
	// Retrieve image at ofBase into cur_image for checksumming
	for (cnt = 0; cnt < ofSize; cnt++) {
	  cur_image[cnt] = NVfetchB(ofBase + cnt);
	}
	
        if (ignore_checksum == 0 && (VerifyChecksum(cur_image) != 0)) {
	  // ...but it was no good.
	  isNewWorld = 0;
	  ofSize = 0;
	}
	break;
      }
      
      // Move along to the next one.
      ofBase += 0x10 * part.length;
    }
    
    if (ofSize == 0) {
      ofBase = 0;
      
      do {	// Loop until we find an image that passes checksum and starts with 0x1275
	ofSize = 0;			// Assume failure
	
	// Find the Open Firmware partition by looking for our magic number
	// as the first two bytes of a page.  We find its size by using the
	// subsequent of_pages field as a count of 256 byte pages in the partition.
	
	for (i = ofBase; i < kNVRAMSize; i += kNVRAMPageSize) {
	  if (((NVfetchB (i) << 8) | NVfetchB (i + 1)) == kOpenFirmwareMagic) {
	    ofBase = i;				// This is the start of O/F partition
	    ofSize = (word) NVfetchB (i + 3) << 8;// Get size from of_pages * page size
	    break;
	  }
	}
	
	if (ofSize > kMaxImageSize || ofSize <= 0) {
	  ofBase += kNVRAMPageSize;
	  continue;
	}
	
	// Retrieve image at ofBase into cur_image for checksumming
	for (i=0; i < ofSize; ++i) {
	  cur_image[i] = NVfetchB (ofBase + i);
	}
	
        if (ignore_checksum == 0 && VerifyChecksum (cur_image) != 0) {
	  ofBase += kNVRAMPageSize;
	  continue;
	} else {
	  break;
	}
      } while (ofBase < kNVRAMSize);
    }
    
    if (ofSize == 0)  {
      fatal(-1, "No Open Firmware NVRAM partition found");
    }
}

//
// Open the NVRAM device and find the OF NVRAM section
//
int
OpenNVRAM()
{
    openHardwareNVRAM ();

    memcpy( new_image, cur_image, ofSize );
#if DUMP
    dump_image();
#endif
    if( VerifyChecksum( cur_image ) ) {
	if (ignore_checksum) {
	    error(-1, "Existing NVRAM fails checksum test");
	} else {
	    fatal(-1, "Existing NVRAM fails checksum test");
	}
    }
    
    if (isNewWorld) OpenNewDict(new_image);
    
    return 0;
}

//
//
//
int
OFsetenv( charP name, charP value )
{
    int i, ndx, ndx1;
    char c;
    word lvalue, bit, nybble, mask;
    half str, len;
    half top, here;
    NewDictPtr dict;
    
    if (debug) {
	error(-1, "setenv '%s' '%s'", name, value);
    }
    
    if (isNewWorld) {
      // See if the variable is already in the table...
      dict = newDict;
      while (dict != NULL) {
	if (strcmp(dict->name, name) == 0) break;
	dict = dict->next;
      }
      if (dict == NULL) {
	// Didn't find one so make a new one.
	dict = (NewDictPtr)malloc(sizeof(NewDict));
	if (dict == NULL) fatal(-1, "Error ran out of memory");
	
	// make the name
	dict->name = (charP)malloc(strlen(name) + 1);
	if (dict->name == NULL) fatal(-1, "Error ran out of memory");
	strcpy(dict->name, name);
	
	// set the value to NULL for the moment...
	dict->value = NULL;
	
	// add it to the list;
	dict->next = NULL;
	if (newDict == NULL) {
	  newDict = dict;
	  newDictLast = dict;
	} else {
	  newDictLast->next = dict;
	  newDictLast = dict;
	}
      } else {
	// free the old value
	free(dict->value);
	dict->value = NULL;
      }
      
      // dict is the right entry so stuff the value;
      dict->length = strlen(value);
      dict->value = (charP)malloc(dict->length);
      if (dict->value == NULL) fatal(-1, "Error ran out of memory");
      memcpy(dict->value, value, dict->length);
      
      // All done...
      wasChanged = 1;
      return 0;
    }

    ndx = 0;
    while( dict_table[ndx].name ) {
	if( strcmp(dict_table[ndx].name,name) == 0 ) {
	    switch( dict_table[ndx].type ) {
	    case kOFboolean:
		if( strcmp( value, "true" ) == 0 ) {
		    bit = 1;
		} else if( strcmp( value, "false" ) == 0 ) {
		    bit = 0;
		} else {
		    return OF_env_ILLEGAL_VALUE;
		}
		mask = ~(1<<(31-dict_table[ndx].offset));
		bit <<= (31-dict_table[ndx].offset);
		lvalue = FetchL( &(((of_hdrP)cur_image)->of_flags) );
		lvalue = (lvalue & mask) | bit;
		StoreL( &(((of_hdrP)cur_image)->of_flags), lvalue );
		return 0;

	    case kOFlong:
		len = strlen( value );
		lvalue = 0;
		for( i = 0; i<len; i++ ) {
		    c = toupper( value[i] );
		    if( isdigit(c) ) {
			nybble = c & 0xF;
		    } else if( isupper(c) && (c < 'G') ) {
			nybble = (c - 7) & 0xF;
		    } else {
			return OF_env_ILLEGAL_VALUE;
		    }
		    lvalue = (lvalue<<4) | nybble;
		}
		StoreL( (wordP)&cur_image[ dict_table[ndx].offset ], lvalue );
		return 0;

	    case kOFstring:
		/*
		 * the case for string update is complicated because
		 * we do "garbage collection" of the string space.
		 */
		memcpy( new_image, cur_image, ofSize );
		ndx1 = 0;
		top = ofSize;
		here = FetchW( &(((of_hdrP)cur_image)->of_here) ) -ofBase;
		memset( &new_image[here], 0, ofSize-here );
		while( dict_table[ndx1].name ) {
		    if( (dict_table[ndx1].type ==3) && ndx1 != ndx ) {
			str = FetchW( (halfP)&cur_image[ dict_table[ndx1].offset ] ) -ofBase;
			len = FetchW( (halfP)&cur_image[ dict_table[ndx1].offset+2 ] );
			top -= len;
			if( len )
			    memcpy( &new_image[top], &cur_image[str], len );
			StoreW( (halfP)&new_image[ dict_table[ndx1].offset ], top +ofBase );
		    }
		    ndx1++;
		}
		len = strlen( value );
		top -= len;
		if( here < top ) {
		    if( len )
			memcpy( &new_image[top], value, len );
		    StoreW( (halfP)&new_image[ dict_table[ndx].offset ], top +ofBase );
		    StoreW( (halfP)&new_image[ dict_table[ndx].offset+2 ], len );
		    StoreW( &(((of_hdrP)cur_image)->of_top), top +ofBase );
		    memcpy( cur_image, new_image, ofSize );
		    return 0;
		} else {
		    return OF_end_NO_ROOM;
		}
		return 0;
	    }
	}
	ndx++;
    }
    return OF_env_INVALID_NAME;
}


int
OFgetenv( charP name, charP value )
{
    int i, ndx;
    word lvalue;
    half str,len;
    NewDictPtr dict;

    if (isNewWorld) {
      // Find the name in the table...
      dict = newDict;
      while (dict != NULL) {
	if (strcmp(dict->name, name) == 0) {
	  memcpy(value, dict->value, dict->length);
	  value[dict->length] = '\0';
	  return 0;
	}
	dict = dict->next;
      }
      
      return OF_env_INVALID_NAME;
    }
    
    ndx = 0;
    while( dict_table[ndx].name ) {
	char str_value[kMaxStringSize];

	if( strcmp(dict_table[ndx].name,name) == 0 ) {
	    switch( dict_table[ndx].type ) {
	    case kOFboolean:
		lvalue = FetchL( &(((of_hdrP)cur_image)->of_flags) );
		if( lvalue & (1 << (31-dict_table[ndx].offset)) )
		    strcpy( value, "true" );
		else
		    strcpy( value, "false" );
		return 0;

	    case kOFlong:
		lvalue = FetchL( (wordP)&cur_image[ dict_table[ndx].offset ] );
		for( i=0; i<8; i++ ) {
		    str_value[i] = "0123456789ABCDEF"[ (lvalue >> (28-4*i)) & 0xF ];
		}
		str_value[8] = 0;
		strcpy( value, str_value );
		return 0;

	    case kOFstring:
		str = FetchW( (halfP)&cur_image[ dict_table[ndx].offset ] ) -ofBase;
		len = FetchW( (halfP)&cur_image[ dict_table[ndx].offset+2 ] );
		if( len )
		    memcpy( str_value, &cur_image[str], len );
		str_value[len] = 0;
		strcpy( value, str_value );
		return 0;
	    }
	}
	ndx++;
    }
    return OF_env_INVALID_NAME;
}

int
CloseNVRAM()
{
    int i;
    
    if (isNewWorld) {
      if (wasChanged) CloseNewDict(cur_image);
    } else {
      ((of_hdrP)cur_image)->of_checksum = 0;
      ((of_hdrP)cur_image)->of_checksum = ~checksum( cur_image );
    }
#if DUMP
    dump_image();
#endif

    if (!isNewWorld || wasChanged) {
      for( i=0; i<ofSize; i++ )
	NVstoreB (ofBase+i, cur_image[i]);
    }

    return 0;
}

void
parsefile(char *filename)
{
    char name[kMaxNameSize];
    char value[kMaxStringSize];
    int c;
    FILE *patches;
    int state;
    int ni = 0;
    int vi = 0;
    int err;

    if ((patches = fopen(filename, "r")) == NULL) {
	fatal(errno, "Couldn't open patch file - '%s'", filename);
    }
    state = kFirstColumn;
    while ((c = getc(patches)) != EOF) {
#if DUMP
	//printf("%s -> '%c'\n", state_names[state], c);
#endif
	switch (state) {
	case kFirstColumn:
	    ni = 0;
	    vi = 0;
	    if (c == '#') {
		state = kScanComment;
	    } else if (c == '\n') {
		// state stays kFirstColumn
	    } else if (isspace(c)) {
		state = kFindName;
	    } else {
		state = kCollectName;
		name[ni++] = c;
	    }
	    break;
	case kScanComment:
	    if (c == '\n') {
		state = kFirstColumn;
	    } else {
		// state stays kScanComment
	    }
	    break;
	case kFindName:
	    if (c == '\n') {
		state = kFirstColumn;
	    } else if (isspace(c)) {
		// state stays kFindName
	    } else {
		state = kCollectName;
		name[ni++] = c;
	    }
	    break;
	case kCollectName:
	    if (c == '\n') {
		name[ni] = 0;
		error(-1, "Name must be followed by white space - '%s'", name);
		state = kFirstColumn;
	    } else if (isspace(c)) {
		state = kFindValue;
	    } else {
		name[ni++] = c;
		// state stays kCollectName
	    }
	    break;
	case kFindValue:
	case kContinueValue:
	    if (c == '\n') {
		state = kSetenv;
	    } else if (isspace(c)) {
		// state stays kFindValue or kContinueValue
	    } else {
		state = kCollectValue;
		value[vi++] = c;
	    }
	    break;
	case kCollectValue:
	    if (c == '\n') {
		if (value[vi-1] == '\\') {
		    value[vi-1] = '\r';
		    state = kContinueValue;
		} else {
		    state = kSetenv;
		}
	    } else {
		// state stays kCollectValue
		value[vi++] = c;
	    }
	    break;
	}
	if (state == kSetenv) {
	    name[ni] = 0;
	    value[vi] = 0;
	    if ((err = OFsetenv (name, value)) != 0) {
		fatal(-1, "Error (%d) setting variable - '%s'", err, name);
	    }
	    state = kFirstColumn;
	}
    }
#if DUMP
    //printf("%s -> EOF\n", state_names[state]);
#endif
    if (state != kFirstColumn) {
	fatal(-1, "Last line ended abruptly");
    }
}

/*
 * set_or_get ( name_value )
 *
 *	Set or get a variable.
 */
void
set_or_get(str)
char *str;
{
    char *name;
    char *value;
    int err;
    int set = 0;
    char get_value[kMaxStringSize];
    char *s;

    name = str;
    while (*str) {
	if (*str == '=') {
	    set = 1;
	    *str++ = 0;
	    break;
	}
	str++;
    }
    if (set) {
	value = str;
	if ((err = OFsetenv (name, value)) != 0) {
	    fatal(-1, "Error (%d) setting variable - '%s'", err, name);
	}
    } else {
	if ((err = OFgetenv (name, get_value)) != 0) {
	    fatal(-1, "Error (%d) getting variable - '%s'", err, name);
	} else {
	    for (s = get_value; *s; s++) {
		if (*s == '\r') {
		    *s = '\n';
		}
	    }
	    printf("%s\t%s\n", name, get_value);
	}
    }

}


void
set_from_table()
{
    int k;
    int err;
    
    for (k = 0; patch_table[k].name != 0; k++) {
	if ((err = OFsetenv (patch_table[k].name, patch_table[k].value)) != 0) {
	    fatal(-1, "Error (%d) setting an Open Firmware NVRAM variable", err);
	}
    }
}

void
OFprintenv()
{
    int err;
    int ndx;
    char *name;
    char get_value[kMaxStringSize];
    char *s;
    NewDictPtr dict;
    int cnt;
    byte tmp;

    if (isNewWorld) {
      // Run through all the names in the table.
      dict = newDict;
      while (dict != NULL) {
	printf("%s\t", dict->name);
	
	// dump the value.
	for (cnt = 0; cnt < dict->length; cnt++) {
	  tmp = dict->value[cnt];
	  if (isprint(tmp)) putchar(tmp);
	  else printf("%%%02x", tmp);
	}
	
	putchar('\n');
	dict = dict->next;
      }
      
      return;
    }
    
    ndx = 0;
    while( name = dict_table[ndx].name ) {
	if ((err = OFgetenv (name, get_value)) != 0) {
	    fatal(-1, "Error (%d) getting variable - '%s'", err, name);
	} else {
	    for (s = get_value; *s; s++) {
		if (*s == '\r') {
		    *s = '\n';
		}
	    }
	    printf("%s\t%s\n", name, get_value);
	}
	ndx++;
    }
}

int
main(argc, argv)
int argc;
char **argv;
{
    register int i;
    register char *s;
    int err;
    char message[100];

    if ((program = strrchr(argv[0], '/')) != (char *)NULL) {
	program++;
    } else {
	program = argv[0];
    }

    debug = 0;

    if ((err = OpenNVRAM ()) != 0)
	fatal(-1, "Error (%d) opening Open Firmware NVRAM", err);

    for (i = 1; i < argc; i++) {
	s = argv[i];
	if (s[0] == '-' && s[1] != 0) {
	    /* options */
	    for (s += 1 ; *s; s++) {
		switch (*s) {

		case 'd':
		    debug = 1;
		    break;

		case 'p':
		    OFprintenv();
		    break;

		case 'i':
		    set_from_table();
		    break;
			
		case 'f':
		    i++;
		    if (i < argc && *argv[i] != '-') {
			    parsefile(argv[i]);
		    } else {
			    usage("missing filename");
		    }
		    break;

		default:
		    strcpy(message, "no such option as --");
		    message[strlen(message)-1] = *s;
		    usage(message);
		}
	    }
	} else {
	    /* Other arguments */
	    set_or_get(s);
	}
    }
    
    if ((err = CloseNVRAM ()) != 0)
	fatal(-1, "Error (%d) closing Open Firmware NVRAM", err);
    return (0);
}


int OpenNewDict(charP buffer)
{
  byte       tmp;
  charP      name, value;
  int        cnt, length;
  NewDictPtr dict;
  
  buffer += 0x10;
  
  while (*buffer != '\0') {
    // Get the name
    name = buffer;
    while (*buffer != '=') buffer++;
    *(buffer++) = '\0';
    
#if 0    
    // First get the length of the value
    cnt = 0;
    length = 0;
    while (buffer[cnt] != '\0') {
      if ((byte)buffer[cnt] == (byte)0x00ff) {
	tmp = buffer[++cnt] & 0x7f;
	length += tmp;
      } else length++;
      cnt++;
    }
    
    // Copy the value
    value = (charP)malloc(length);
    if (value == NULL) fatal(-1, "Error ran out of memory");
    length = 0;
    while (*buffer != '\0') {
      if ((byte)*buffer == (byte)0x00ff) {
	tmp = *(++buffer);
	for (cnt = 0; cnt < tmp & 0x7f; cnt++)
	  value[length++] = (tmp & 0x80) ? 0xff : 0x00;
      } else value[length++] = *buffer;
      buffer++;
    }
#else
    length = strlen(buffer);
    // Copy the value
    value = (charP)malloc(length + 1);
    if (value == NULL) fatal(-1, "Error ran out of memory");
    strcpy(value, buffer);
    buffer += length;
#endif
    
    // make the new dict entry
    dict = (NewDictPtr)malloc(sizeof(NewDict));
    if (dict == NULL) fatal(-1, "Error ran out of memory");
    
    // make the name
    dict->name = (charP)malloc(strlen(name) + 1);
    if (dict->name == NULL) fatal(-1, "Error ran out of memory");
    strcpy(dict->name, name);
    
    // make the value;
    dict->length = length;
    dict->value = value;
    
    // add it to the list;
    dict->next = NULL;
    if (newDict == NULL) {
      newDict = dict;
      newDictLast = dict;
    } else {
      newDictLast->next = dict;
      newDictLast = dict;
    }
    
    // move buffer to the start of the next entry.
    buffer++;
  }
  
  return 0;
}


int CloseNewDict(charP buffer)
{
  NewDictPtr dict = newDict;
  int        num, cnt, pos = 0x10;
  byte       tmp;
  
  // Start off clean.
  memset(buffer + 0x10, 0x00, ofSize - 0x10);
  
  while (dict != NULL) {
    // Check to make sure the last name did not overflow.
    if (pos >= ofSize) fatal(-1, "Error out of NVRAM Space.");

    // save the name.
    strcpy(buffer + pos, dict->name);
    pos += strlen(dict->name);
    
    // save the '='.
    buffer[pos++] = '=';
    
    // save the value.
    for (cnt = 0; cnt < dict->length; cnt++) {
#if 0
      tmp = dict->value[cnt];
      if ((tmp == 0x00) || (tmp == 0xff)) {
	buffer[pos++] = 0xff;
	num = 1;
	while ((num < 0x80) && (cnt < dict->length) &&
	       (dict->value[cnt] == tmp)) {
	  num++;
	  cnt++;
	}
	buffer[pos++] = (tmp & 0x80) | (num && 0x7f);
      } else buffer[pos++] = tmp;
#else
      buffer[pos++] = dict->value[cnt];
#endif
    }
    
    // Save the '\0'.
    buffer[pos++] = '\0';

    dict = dict->next;
  }
  
  // Save the final '\0';
  buffer[pos] = '\0';
  
  return 0;
}

