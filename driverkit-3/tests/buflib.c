/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Standard buffer dump/compare library.
 */
 
#import <bsd/sys/types.h>
#import <bsd/libc.h>
#import "buflib.h"

int buf_comp(int size, u_char *wbuf, u_char *rbuf) 
{
	/* verify wbuf == rbuf for size bytes */
	
	register int i;
	char inch;
	int bufad;
	u_char *bp;
	
	for(i=0; i<size; i++) {
		if(wbuf[i] != rbuf[i]) {
			printf("***DATA ERROR***\n");
			printf("byte %XH\n",i);
			printf("Expected data = %02XH   Received = %02XH\n",
				wbuf[i],rbuf[i]);
			while(1) {
			    printf("More (m), buffer dump (d), quit (q))? ");
			    inch = getchar();
			    getchar();
			    switch(inch) {
				case 'q':
				    return(1);
				case 'm':
				    goto next_int;
				case 'd':
				    printf("Which buffer (r/w)? ");
				    inch = getchar();
				    getchar();
				    switch(inch) {
					case 'r':
					    bp = rbuf;
					    break;
					case 'w':
					    bp = wbuf;
					    break;
					default:
					    printf("Huh??\n");
					    continue;
				    }
				    printf("Enter buffer address: ");
				    bufad = get_num(0, 16);
				    dump_buf((u_char *)&bp[bufad], size);
				    break;
			    }
			}
next_int:
			continue;
	        }
	}
	return(0);
} /* buf_comp() */

int get_num(int deflt, 
		int base)	/* HEX or DEC, user can override with 'x' or 
				 *   '0x' */
{
	char x_bad=0;
	int digit;
	char inch;
	int first_char=1;
	int result;
	
	/* 
	 * returns new number, if a legal one entered, else 'deflt'. 
	 * Radix assumes to be 'base'; can be overridden by leading
	 * '0x'.
	 */
	
	result = 0;
	while(1) {
		inch = getchar();
		if(inch == '\n') {
			if(first_char) {
				result = deflt;
				return(result);
			}
			else
				return(result);
		}
		first_char = 0;
		if(inch == 'x') {
			if(x_bad) 
				goto nullret;
			else {
				base = HEX;
				x_bad++;
			}
		}
		else {
			if((inch >= '0') && (inch <= '9')) 
				digit = inch - '0';
			else {
				inch |= 0x20;	/* make lower case */
				if((inch >= 'a') && (inch <= 'f')) {
					if(base != HEX)		/* hex OK? */ 
						goto nullret;
				}
				else 
					goto nullret;	/* illegal char */
				digit = (inch - 'a') + 10;
			}
			if(digit>0)
				x_bad++;
			result *= base;
			result += digit;
		}
	}

nullret:
	while(getchar() != '\n')
		;
	return(deflt);
	
} /* get_num() */


void dump_buf(u_char *buf, int size) {

	int i;
	char c[100];
	
	printf("\n");
	for(i=0; i<size; i++) {
		if((i>0) && (i%0x100 == 0)) {
			printf("\n...More? (y/anything) ");
			gets(c);
			if(c[0] != 'y')
				return;
		}
		if(i%0x10 == 0)
			printf("\n %03X: ",i);
		else if(i%8 == 0)
			printf("  ");
		printf("%02X ",buf[i]);
	}
	printf("\n");
} /* dump_buf() */ 

/*
 * Get default values from environment (first choice) or specified default.
 */
void get_default(char *name,
	u_int *value,
	u_int deflt)
{
	char *ep;
	
	ep = getenv(name);
	if(ep)
		*value = atoi(ep);
	else
		*value = deflt;
}


void get_default_t(char *name,
	char **value,
	char *deflt)
{
	char *ep;
	
	ep = getenv(name);
	if(ep)
		*value = ep;
	else
		*value = deflt;
}





