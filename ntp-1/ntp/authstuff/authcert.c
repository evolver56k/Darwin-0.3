/*
 * This file, and the certdata file, shamelessly stolen
 * from Phil Karn's DES implementation.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef DES
#define	DES
#endif

#include "ntp_stdlib.h"

u_char ekeys[128];
u_char dkeys[128];

static	void	get8	(u_int32 *);
static	void	put8	(u_int32 *);

int
main(
	int argc,
	char *argv[]
	)
{
	u_int32 key[2], plain[2], cipher[2], answer[2], temp;
	int i;
	int test;
	int fail;

	for(test = 0; !feof(stdin); test++){
		get8(key);
		DESauth_subkeys(key, ekeys, dkeys);
		printf(" K: "); put8(key);
		get8(plain);
		printf(" P: "); put8(plain);
		get8(answer);
		printf(" C: "); put8(answer);
		for (i = 0; i < 2; i++)
		    cipher[i] = htonl(plain[i]);
		DESauth_des(cipher, ekeys);
		for (i = 0; i < 2; i++) {
			temp = ntohl(cipher[i]);
			if (temp != answer[i])
			    break;
		}

		fail = 0;
		if (i != 2) {
			printf(" Encrypt FAIL");
			fail++;
		}
		DESauth_des(cipher, dkeys);
		for (i = 0; i < 2; i++) {
			temp = ntohl(cipher[i]);
			if (temp != plain[i])
			    break;
		}
		if (i != 2) {
			printf(" Decrypt FAIL");
			fail++;
		}
		if (fail == 0)
		    printf(" OK");
		printf("\n");
	}
}

static void
get8(
	u_int32 *lp
	)
{
	int t;
	u_int32 l[2];
	int i;

	l[0] = l[1] = 0;
	for (i = 0; i < 8; i++) {
		scanf("%2x", &t);
		if (feof(stdin))
		    exit(0);
		l[i / 4] <<= 8;
		l[i / 4] |= (u_int32)(t & 0xff);
	}
	*lp = l[0];
	*(lp + 1) = l[1];
}

static void
put8(
	u_int32 *lp
	)
{
	int i;

	for(i = 0; i < 2; i++)
	    printf("%08lx", (u_long)(*lp++));
}
