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
/*
 * fbshow -- show text strings and images in frame buffer
 *
 * FIXME: Don't hack this pile of shit any more, rewrite it.
 *
 * Mike DeMoney, NeXT Inc.
 * Copyright 1990.  All rights reserved.
 *
 * TODO:  Add option to display arbitrary eps images.
 *	Ask Keith how to calculate default lead.
 *	Ask Keith preferred format of arbitrary images.
 *	Get images for boot panel.
 * 
 * fbshow [-f FONT] [-d FONTDIR] [-p POINT] [-x XPOS] [-y YPOS] [-h HEIGHT]
 *	[-w WIDTH] [-l LEAD] [-g BGRNDCOLOR] [-c FRGRNCOLOR] 
 *	[-s LINES_TO_CENTER] [-b BUNDLE_ROOT] [-t STRINGFILE_NAME] 
 *	[-m X_MARGIN] [-z PERCENTDONE] [-A] [-S] [-N] [-M] [-E] [-I] [MSGS]
 */

/* 
 * Garth Snyder	12/9/91
 *
 *	Added -I option for showing localized strings.  -I turns on
 *	a mode which translates strings using the table Bootstrap.strings 
 *	under the bundle rooted at /usr/lib/NextStep/Resources.  Because
 *	formatting differs between languages, all lines should be packed
 *	into a single string with '*' used to indicate a newline.  Although
 *	-s and -I are not incompatible, they do interact as follows: if 
 *	-s is specified, layout is planned for the number of lines specified
 *	and no further layout is done.  If there is no -s clause, layout
 *	is done dynamically by looking to see how many lines (i.e. *'s)
 *	are packed into the localizable string argument.  For monofont
 *	messages, -I without -s is the right way to go; this allows 
 *	the number of lines to change with language.  To get more than
 *	one font into a message, the number of lines must be specified
 *	manually; however, some redistribution is still possible.
 *	For example:
 *
 *	fbshow -s 4 -I -f Helvetica-Bold "Self-destruct failed!"
 *	   -f Helvetica "*The operation was cancelled*by aliens from mars."
 *
 *	Provides for one line of bold text and three of normal text with
 *	flexible line breaks.
 *
 *	The -b option may be used to root the localization bundle at a 
 *	different path, and -t used to specify a different string table name.
 *
 * Garth Snyder 4/29/92
 *
 *	Changed font encoding scheme to include all characters.
 */

/*
 * Sam Streeper 93/06/02
 *
 * Customized for NS/Intel and new loginpanel look.  A bunch of the old code isn't
 * really useful anymore, and should be pruned.
 */

/*
 * 97/07/10
 *
 * This dog still lives. Added 8-bit support for PowerPC. Lots of things
 * don't appear to work in the way of options. All text must have -I.
 */
 
#import <stdio.h>
#import <ctype.h>
#import <stdarg.h>
#import <mach/mach.h>
#import <libc.h>
#import <sys/param.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/file.h>
#import <mach/boolean.h>
#import <sys/ioctl.h>
#import <objc/NXBundle.h>

#define KERNEL_PRIVATE 1
#import <bsd/dev/kmreg_com.h>
#import "font.h"

#define	BITSPIXEL		2
#define PIXELS_PER_BYTE		(8/BITSPIXEL)
#define CONSOLE			"/dev/console"

#define	BLACK_PX		3
#define	DKGRAY_PX		2
#define	LTGRAY_PX		1
#define	WHITE_PX		0

#define DEFAULT_BUNDLE_PATH	"/System/Library/CoreServices/Resources"
#define DEFAULT_STRING_TABLE	"Bootstrap"

#ifdef ppc

#define useColor 1
// On ppc we crop out the color panel borders, they're already drawn
#define PANELWIDTH	(352 - 32)
#define PANELHEIGHT	(264 - 32)
#define TEXTBASELINE	(156 - 16 + 21)
#define THERMTOP	(150 - 16 + 4)
#define TEXT_BG		WHITE_PX
#define TEXT_FG		BLACK_PX

#else

#define PANELWIDTH	352
#define PANELHEIGHT	264
#define TEXTBASELINE	186
#define THERMTOP	150
#define TEXT_BG		LTGRAY_PX
#define TEXT_FG		DKGRAY_PX

#endif

// psuedo VGA coords
#define PANELTOP	((480 - PANELHEIGHT) / 2)
#define PANELLEFT	((640 - PANELWIDTH) / 2)

#define THERMWIDTH 	192
#define THERMHEIGHT 	12

// relative to panel origin
#define THERMLEFT	((PANELWIDTH - THERMWIDTH) / 2)



int height = PANELHEIGHT;
int yorg = PANELTOP;
int width = PANELWIDTH;
int xorg = PANELLEFT;

typedef enum {
	ANIMATE, SUSPEND_ANIMATION, CLEAR_ANIMATION
} animate_t;

/*
 * Command line parameters and defaults
 */
const char *font = "Charcoal";
const char *fontdir = "/usr/lib/bootimages/";
int point = 12;

int percentDone;

int bg = LTGRAY_PX;
int fg = BLACK_PX;
int lead;
boolean_t lead_set = FALSE;
boolean_t autonl = TRUE;
animate_t animate = CLEAR_ANIMATION;
int xmargin = 30;
int ymargin = 0;


/*
 * The current position
 */
int xpos;
int ypos;

/*
 * Internal types
 */
typedef struct thought thought_t;
struct thought {
	char *filename;
	font_t *fontp;
	thought_t *next;
};

/*
 * General globals
 */
unsigned char *fb;			/* pseudo-frame buffer */
int fb_dev = -1;			/* The bitmap console device */
font_t *fontp;				/* current font */
const char *program_name;		/* program name for error messages */
thought_t *thoughts = NULL;
BOOL didCenter = NO;

/*
 * Internal procedure declarations
 */
static int atob(const char *str);
static void blit_string(const char *str);
static void blit_localized_string(const char *str, const char *path, 
    const char *table);
static void blit_bm(bitmap_t *bmp);
static font_t *open_font(const char *fontname);
static void blit_bm(bitmap_t *bmp);
static void fatal(const char *format, ...);
static void check_console(void);
static void open_fb(char *);
static void center(int lines);
static font_t *recollect(char *fontname);
static void remember(char *fontname, font_t *fontp);
static char *newstr(char *string);
static unsigned char * bm_malloc( unsigned size , int justGray);
static void image_bitmap();
    
/*
 * Internal inline procedures
 */
static inline bitmap_t *getbm(unsigned char c);
static inline int bmbit(bitmap_t *bmp, int x, int y);
static inline void setfbbit(int x, int y, int v);
static inline void flushbits(void);
static inline unsigned digit(char c);
static inline void *ckmalloc(int len);
static inline char *newstr(char *string);
/*
 * Handy internal macros
 */
#define	streq(a, b)		(strcmp(a, b) == 0)
#define	NEW(type, num)	((type *)ckmalloc(sizeof(type)*(num)))

void
main(int argc, const char * const argv[])
{
	const char 	*arg = NULL, *argp;
	char 		c;
	const char	*bundlePath = DEFAULT_BUNDLE_PATH;
	const char	*stringTable = DEFAULT_STRING_TABLE;
	BOOL		doLocalize = NO;
	
 	program_name = *argv++; argc--;

	// SD: always check console mode - 
	//     ConsoleMessgae() in hostconfig doesn't have -B
	check_console();

	/* Parse command line args */
	while (argc > 0) {
		if (**argv == '-') {
			argp = *argv++ + 1; argc--;
			while (c  = *argp++) {
				if (islower(c)) {
					if (argc < 1)
						fatal("Option %c takes arg", c);
					arg = *argv++; argc--;
				}
				switch (c) {
				case 'A':
					animate = ANIMATE;
					break;
				case 'S':
					animate = SUSPEND_ANIMATION;
					break;
				case 'g':
					bg = atob(arg);
					break;
				case 'f':
					font = arg;
					break;
				case 'p':
					point = atob(arg);
					break;
				case 'z':
					percentDone = atob(arg);
					break;
				case 'x':
					xorg = atob(arg);
					break;
				case 'y':
					yorg = atob(arg);
					break;
				case 'h':
					height = atob(arg);
					break;
				case 'w':
					width = atob(arg);
					break;
				case 'l':
					lead = atob(arg);
					lead_set = 1;
					break;
				case 'L':
					lead_set = 0;
					break;
				case 'c':
					fg = atob(arg);
					break;
				case 'd':
					fontdir = arg;
					break;
				case 'm':
					xmargin = atob(arg);
					break;
				case 'M':
					autonl = FALSE;
					break;
				case 'N':
					autonl = TRUE;
					break;
				case 'E':
#ifdef m68k
					erase_rom_panel();
#endif
					break;
				case 'B':
					check_console();
					break;
				case 's':
					didCenter = YES;
					center(atob(arg));
					break;
				case 'b':
					bundlePath = arg;
					break;
				case 't':
					stringTable = arg;
					break;
				case 'I':
					doLocalize = YES;
					break;
				default:
					fatal("Unknown option: %c", c);
				}
			}
		} else {
		    if (doLocalize) {
			blit_localized_string(*argv, bundlePath, stringTable);
			argv++; argc--;
		    } else {
			/* Blit the string */
			blit_string(*argv++); argc--;
		    }
		}
	}
	image_bitmap();	/* Draw the offscreen bitmap onto the display */
}

static void
blit_localized_string(const char *str, const char *path, const char *table)
{
    NXBundle	*bundle;
    const char	*localStr;
    char	*buff;
    char	*star;
    char	*mem;
    int		numLines;
    
    if (!(bundle = [[NXBundle alloc] initForDirectory:path])) {
	fatal("Cannot open bundle: %s", path);
    }
    localStr = NXLocalStringFromTableInBundle(table, bundle, str, NULL, NULL);
    if (!localStr) {
	fatal("Unable to localize: %s", str);
    }
    mem = buff = NXZoneMalloc(NXDefaultMallocZone(), strlen(localStr) + 1);
    strcpy(buff, localStr);
    if (!didCenter) {
	numLines = 1;
	star = index(buff, '*'); 
	while (star) {
	    star = index(star + 1, '*');
	    numLines++;
	}
	center(numLines);
	didCenter = YES;
    }
    do {
	if (star = index(buff, '*')) *(star++) = '\0';
	blit_string(buff);
    } while (buff = star);
    [bundle free];
    free(mem);
}

#define TW THERMWIDTH
#define TH THERMHEIGHT

void
image_bitmap()
{
	struct km_drawrect rect;
	
	if ( fb_dev == -1 )
		open_fb( CONSOLE );
	if ( fb == NULL )
		return;		/* Nothing to image??? */
	
#ifdef useColor
	drawThermometerColor();

	rect.y = yorg + THERMTOP + THERMHEIGHT;
	rect.height = height - (THERMTOP + THERMHEIGHT);
	rect.data.bits = (void *) fb + (THERMTOP + THERMHEIGHT) * (width / PIXELS_PER_BYTE);

//	rect.y = yorg;
//	rect.height = height;
//	rect.data.bits = (void *)fb;
#else
	drawThermometerDither();

	rect.y = yorg;
	rect.height = height;
	rect.data.bits = (void *)fb;
#endif
	rect.x = xorg;
	rect.width = width;

	if ( ioctl( fb_dev, KMIOCDRAWRECT, &rect ) == -1 )
		perror( "KMIOCDRAWRECT" );
}

#ifdef useColor
drawThermometerColor()
{

    int x,y;
    unsigned char * line;
    int barPix, grayPix;
    struct km_drawrect rect;

    static const unsigned char barBegin[TH-2][3] = { 
	{ 0xff, 0x7f, 0x7f },
	{ 0xff, 0x7f, 0x54 },
	{ 0xff, 0x7f, 0x2a },
	{ 0xff, 0x7f, 0x00 },
	{ 0xff, 0x7f, 0x00 },
	{ 0xff, 0x7f, 0x00 },
	{ 0xff, 0x7f, 0x2a },
	{ 0xff, 0x7f, 0x54 },
	{ 0xff, 0x7f, 0x7f },
	{ 0xff, 0x7f, 0xaa }
    };
    static const unsigned char barFill[TH-2] = { 0xaa, 0x7f, 0x54, 0x2a, 0x00, 0x2a, 0x54, 0x7f, 0xaa, 0xf1 };
    static const unsigned char barEnd[TH-2][6] = { 
	{ 0xaa, 0xaa, 0xaa, 0xff, 0xfb, 0xf9 }, 
	{ 0x7f, 0x7f, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0x54, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0x2a, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0x2a, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 

	{ 0x2a, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0x54, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0x7f, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0xaa, 0xaa, 0xf1, 0xff, 0xfb, 0xf9 }, 
	{ 0xf1, 0xf1, 0xf1, 0xff, 0xfb, 0xf9 }
    };
    static const unsigned char grayFill[TH-2] = { 0xf9, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf6 };
    static const unsigned char grayEnd[TH-2][2] = { 
	{ 0xf8, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff },
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
	{ 0xf6, 0xff }, 
    };

    if (percentDone < 0 || percentDone > 100) return;

    barPix = (((TW - 3 - 6 - 2) * percentDone)/100);
    grayPix = (TW - 3 - 6 - 2) - barPix;

    if( grayPix == 0)
	barPix += 4;

    line = fb;
    memset( line, 0xff, TW );
    line += TW;
    for (y=0; y<(TH-2); y++) {

	memmove( line, barBegin[y], 3 );
	x = 3;
	memset( line + x, barFill[y], barPix );
	x += barPix;

	if( grayPix) {
	    memmove( line + x, barEnd[y], 6 );
	    x += 6;
	    memset( line + x, grayFill[y], grayPix );
	    x += grayPix;
	    memmove( line + x, grayEnd[y], 2 );

	} else {
	    memmove( line + x, barEnd[y], 4 );
	}
	line += TW;
    }
    memset( line, 0xff, TW );
    line += TW;

    rect.x = (xorg + THERMLEFT) | 3;
    rect.width = TW;
    rect.y = (yorg + THERMTOP);
    rect.height = TH;
    rect.data.bits = (void *)fb;		// reuse that big blank area - make sure it fits!

    if ( ioctl( fb_dev, KMIOCDRAWRECT, &rect ) == -1 )
	    perror( "KMIOCDRAWRECT" );

}

#else

void cpy2( int x, int y, const unsigned char * src, int len )
{
    int i;

     for( i = 0; i < len; i++)
	setfbbit( THERMLEFT + x + i, THERMTOP + y, *(src + i));
}

drawThermometerDither()
{
    int x,y,i;
    int barPix, grayPix;

    static const unsigned char barBegin[TH][3] = { 
	{ 3, 3, 3 },
	{ 3, 2, 2 },
	{ 3, 2, 1 },
	{ 3, 2, 0 },
	{ 3, 2, 0 },
	{ 3, 2, 0 },
	{ 3, 2, 0 },
	{ 3, 2, 0 },
	{ 3, 2, 1 },
	{ 3, 2, 2 },
	{ 3, 2, 2 },
	{ 3, 3, 3 },
    };
    static const unsigned char barFill[ 2 ][TH] = {
	{ 3, 2, 2, 1, 1, 0, 0, 1, 2, 2, 3, 3 },
	{ 3, 2, 2, 1, 0, 0, 1, 1, 2, 2, 3, 3 }
    };

    static const unsigned char barEnd[TH][5] = { 
	{ 3, 3, 3, 3, 3 }, 
	{ 2, 2, 3, 2, 2 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 2, 3, 3, 2, 1 }, 
	{ 3, 3, 3, 2, 1 }, 
	{ 3, 3, 3, 3, 3 }, 
    };

    static const unsigned char grayFill[TH] = { 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 3 };
    static const unsigned char grayEnd[TH][2] = { 
	{ 3, 3 },
	{ 1, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 3, 3 },
    };

    if (percentDone < 0 || percentDone > 100) return;

    barPix = (((TW - 3 - 5 - 2) * percentDone)/100);
    grayPix = (TW - 3 - 5 - 2) - barPix;
    if( grayPix == 0)
	barPix += 4;

    for (y=0; y<(TH); y++) {
	cpy2( 0, y, barBegin[y], 3 );
	x = 3;
	for( i = 0; i < barPix; i++) {
	    setfbbit(THERMLEFT + x, THERMTOP + y, barFill[i & 1][y] );
	    x++;
	}
	if( grayPix) {
	    cpy2( x, y, barEnd[y], 5 );
	    x += 5;
            for( i = 0; i < grayPix; i++) {
                setfbbit(THERMLEFT + x, THERMTOP + y, grayFill[y] );
                x++;
            }
	    cpy2( x, y, grayEnd[y], 2 );
	} else {
	    cpy2( x, y, barEnd[y], 3 );
	}
    }
    flushbits();
}

#endif

static void
center(int lines)
{
	int deflead, space;

	if (lines <= 0)
		return;	
	if (fontp == NULL || strcmp(font, fontp->font) != 0)
		fontp = open_font(font);
		
	/* Round up origin points. */
	xorg = (xorg + PIXELS_PER_BYTE - 1) & ~(PIXELS_PER_BYTE - 1);
	/* Round down width */
	width &= ~(PIXELS_PER_BYTE - 1);
	
#ifdef NATURAL_ALIGNMENT
	deflead = (int)(fontp->bbx.height + (fontp->bbx.height + 9) / 10.0);
#else
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;
#endif /* NATURAL_ALIGNMENT */

	space = height - lines * (lead_set ? lead : deflead);
	if (lead_set)
		ymargin = space / 2;
	else {
		space /= 6 + lines - 1;
		ymargin = space * 3 + 9;
		lead = deflead + space;
		lead_set = TRUE;
	}

	ymargin = TEXTBASELINE - (fontp->bbx.height - 1);
}

static void
check_console(void)
{
	int cfd;
	int kmflags;
	
	if ((cfd = open("/dev/console", O_RDONLY, 0)) < 0)
		exit(0);
	if (ioctl(cfd, KMIOCSTATUS, &kmflags) < 0)
		exit(0);
	if (kmflags & KMS_SEE_MSGS)
		exit (0);
	(void) close (cfd);
}

static font_t *
open_font(const char *fontname)
{
	char filename[MAXPATHLEN];
	char *slash;
	struct stat st;
	kern_return_t result;
	int ffd;
	font_t *fontp;
	
	slash = (fontdir[strlen(fontdir)-1] == '/') ? "" : "/";
// xxx fixme should be the ifdef for little endian
#ifdef i386
	sprintf(filename, "%s%s%s.%d.le", fontdir, slash, fontname, point);
#else i386
	sprintf(filename, "%s%s%s.%d.be", fontdir, slash, fontname, point);
#endif
	if (fontp = recollect(filename))
		return fontp;
	if ((ffd = open(filename, O_RDONLY, 0)) < 0)
		fatal("Can't open font file %s", filename);
	if (fstat(ffd, &st) < 0)
		fatal("Can't stat font file %s", filename);
	result = map_fd(ffd, (vm_offset_t)0, (vm_offset_t *)&fontp, TRUE, st.st_size);
	if (result != KERN_SUCCESS)
		fatal("Can't map font file %s", filename);
	close(ffd);
#if 0 // NATURAL_ALIGNMENT
	{
	    int i, size = sizeof(bbox_t) + sizeof(short);
	    font_t *f = fontp;
	    char   *p = (char *)f;
	    fontp = malloc(sizeof(font_t) + st.st_size); // a bit wasteful but...
	    if (fontp == NULL) fatal("Can't allocate font struct %s", filename);
	    memcpy(&fontp->font[0],&f->font[0],sizeof(f->font));
	    fontp->size = f->size;

	    fontp->bbx  = f->bbx;
	    p = (char *)&(f->bitmaps[0]);

	    for (i = 0; i < (sizeof(f->bitmaps)/sizeof(f->bitmaps[0])); i++) {
		bitmap_t *bit = &fontp->bitmaps[i];
		memcpy((char *)bit,p,size);
                p += size;
		memcpy((char *)&bit->bitx,p,sizeof(int));
		p += sizeof(int);
	    }
	    memcpy(&fontp->bits[0],p, st.st_size - ((int)p-(int)f));
	}
#endif /* NATURAL_ALIGNMENT */
	remember(filename, fontp);
	return fontp;
}

static inline void *
ckmalloc(int len)
{
	void *p;
	
	p = malloc(len);
	if (p == NULL)
		fatal("Out of memory");
	return p;
}

static inline char *
newstr(char *string)
{
	int len;
	char *newstrp;
	
	len = strlen(string) + 1;
	newstrp= (char *)ckmalloc(len);
	if (newstrp == NULL)
		fatal("Out of memory");
	bcopy(string, newstrp, len);
	return newstrp;
}

static void
remember(char *filename, font_t *fontp)
{
	thought_t *tp;
	
	if (recollect(filename))
		return;
	tp = NEW(thought_t, 1);
	tp->filename = newstr(filename);
	tp->fontp = fontp;
	tp->next = thoughts;
	thoughts = tp;
}

static font_t *
recollect(char *filename)
{
	thought_t *tp;
	
	for (tp = thoughts; tp; tp = tp->next)
		if (streq(tp->filename, filename))
			return tp->fontp;
	return NULL;
}

static inline bitmap_t *
getbm(unsigned char c)
{
	return &fontp->bitmaps[c - ENCODEBASE];
}

/*
 * Currently, this function just calls /dev/vid0 to control the animation.
 * The actual drawing code is driven off of /dev/console.
 */
static void
open_fb(char *dev)
{
	km_anim_ctl_t anim_ctl;
	
	switch(animate) {
	    case ANIMATE:
	    	anim_ctl = KM_ANIM_RESUME;
		break;
		
	    case SUSPEND_ANIMATION:
	    	anim_ctl = KM_ANIM_SUSPEND;
		break;
	    default:
	    case CLEAR_ANIMATION:
	    	anim_ctl = KM_ANIM_STOP;
		break;
	}
	if ((fb_dev = open(dev, O_WRONLY, 0)) < 0)
		fatal("Can't open console %s", CONSOLE);
	if (ioctl(fb_dev, KMIOCANIMCTL, &anim_ctl) < 0)
	{
#ifndef i386
//		fatal("Can't set animation state");
#endif i386
	}
}

static void
blit_string(const char *str)
{
	int c;
	bitmap_t *bmp;
	int deflead;
	static boolean_t first_line = TRUE;
	const unsigned char *string = str;
	
	if ( fb_dev == -1 )
		open_fb( CONSOLE );
	if (fontp == NULL || strcmp(font, fontp->font) != 0)
		fontp = open_font(font);
		
	/* Round up origin points. */
	xorg = (xorg + PIXELS_PER_BYTE - 1) & ~(PIXELS_PER_BYTE - 1);
	/* Round down width and height */
	width &= ~(PIXELS_PER_BYTE - 1);
	
#ifdef NATURAL_ALIGNMENT
	deflead = (int)(fontp->bbx.height + (fontp->bbx.height + 9) / 10.0);
#else
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;
#endif /* NATURAL_ALIGNMENT */
	if (first_line) {
		first_line = FALSE;
		xpos = xmargin;
		ypos = ymargin + fontp->bbx.height;
	}

	// calculate xoffset to center text.
	// xxx fixme - does the wrong thing if newlines included
	xpos = width;
	while (c = *string++) {
		bmp = getbm(c);
		xpos -= bmp->dwidth;
	}
	xpos /= 2;
	string = str;

	while (c = *string++) {
		if (c == '\\') {
			switch (c = *string++) {
			case 'n':
				c = '\n';
				break;
			}
		}				
		if (c == '\n') {
			xpos = 0;
			ypos += lead_set ? lead : deflead;
			if( ypos > (height-16))
			    break;
			continue;
		}
		bmp = getbm(c);
		blit_bm(bmp);
	}
	if (autonl) {
		xpos = xmargin;
		ypos += lead_set ? lead : deflead;
	}
}

static inline int
bmbit(bitmap_t *bmp, int x, int y)
{
	int bitoffset;
	
	bitoffset = bmp->bitx + y * bmp->bbx.width + x;
	return (fontp->bits[bitoffset >> 3] >> (7 - (bitoffset & 0x7))) & 1;
}

static unsigned char *lastfbaddr;
static unsigned char fbbyte;

/*
 * Set a bit in the 'frame buffer'.  Assumes that left-most displayed pixel
 * is most significant in the frame buffer byte.  Also that 0,0 is at lowest
 * address of frame buffer.
 */
static inline void
setfbbit(int x, int y, int color)
{
	unsigned char *fbaddr;
	int bitoffset;
	int mask;
	int bitshift;

	if (fb == NULL)
		fb = bm_malloc( (height * width * BITSPIXEL) / 8 , 0);
	bitoffset = ((y * width) + x) * BITSPIXEL;
	fbaddr = fb + (bitoffset >> 3);
	if (fbaddr != lastfbaddr) {
		if (lastfbaddr)
			*lastfbaddr = fbbyte;
		fbbyte = *fbaddr;
		lastfbaddr = fbaddr;
	}
	bitshift = (8 - BITSPIXEL) - (bitoffset & 0x7);
	color <<= bitshift;
	mask = ~(-1 << BITSPIXEL) << bitshift;
	fbbyte = (fbbyte & ~mask) | (color & mask);
}

static inline void
flushbits(void)
{
	if (lastfbaddr)
		*lastfbaddr = fbbyte;
}

/*
 * Assumes bitmap is in PS format, ie. origin is lower left corner.
 */
static void
blit_bm(bitmap_t *bmp)
{
	int x, y;
	/*
	 * x and y are in fb coordinate system
	 * xoff and yoff are in ps coordinate system
	 */
	for (y = 0; y < bmp->bbx.height; y++)
	{
		for (x = 0; x < bmp->bbx.width; x++)
		{
			int theBit = bmbit(bmp, x, (bmp->bbx.height - y - 1));
			if (theBit) setfbbit(xpos + bmp->bbx.xoff + x, 
					ypos - bmp->bbx.yoff - y, TEXT_FG);
		}
	}
	flushbits();
	xpos += bmp->dwidth;
}

static unsigned char *
bm_malloc( unsigned size , int justGray)
{
	unsigned char * bitmap = (unsigned char *)malloc( size );
	unsigned char value;
	unsigned int i;
	char filename[MAXPATHLEN];
	int ifd;
	int color;

	color = TEXT_BG;
//	color = justGray ? justGray : bg;

#ifndef useColor
	if ( bitmap == (unsigned char *) NULL )
		fatal( "malloc: couldn't allocate %d bytes.\n" );

	sprintf(filename, "%s/%s", fontdir, "LoginTwo.data");
	if ((!justGray) && ((ifd = open(filename, O_RDONLY, 0)) >= 0))
	{
		read(ifd, bitmap, size);
		close(ifd);
	}
	else
#endif
	{
	    /* Sleazoid clear to background */
	    value = 0;
	    for ( i = 0; i < PIXELS_PER_BYTE; ++i )
		    value |= (color << (i * BITSPIXEL));
	    memset( bitmap, value, size);
	}
	return ( bitmap );
}

/*
 * digit -- convert the ascii representation of a digit to its
 * binary representation
 */
static inline unsigned
digit(char c)
{
	unsigned d;

	if (isdigit(c))
		d = c - '0';
	else if (isalpha(c)) {
		if (isupper(c))
			c = tolower(c);
		d = c - 'a' + 10;
	} else
		d = 999999; /* larger than any base to break callers loop */

	return(d);
}

/*
 * atob -- convert ascii to binary.  Accepts all C numeric formats.
 */
static int
atob(const char *cp)
{
	int minus = 0;
	int value = 0;
	unsigned base = 10;
	unsigned d;

	if (cp == NULL)
		return(0);

	while (isspace(*cp))
		cp++;

	while (*cp == '-') {
		cp++;
		minus = !minus;
	}

	/*
	 * Determine base by looking at first 2 characters
	 */
	if (*cp == '0') {
		switch (*++cp) {
		case 'X':
		case 'x':
			base = 16;
			cp++;
			break;

		case 'B':	/* a frill: allow binary base */
		case 'b':
			base = 2;
			cp++;
			break;
		
		default:
			base = 8;
			break;
		}
	}

	while ((d = digit(*cp)) < base) {
		value *= base;
		value += d;
		cp++;
	}

	if (minus)
		value = -value;

	return value;
}

static void
fatal(const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	fprintf(stderr, "%s: ", program_name);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

#ifdef m68k
erase_rom_panel()
{
	xorg = 340;
//	yorg = 328;
	yorg = 308;
	height = 176;
	width = 440;
	fb = bm_malloc( (height * width * BITSPIXEL) / 8 , 1);
}
#endif














