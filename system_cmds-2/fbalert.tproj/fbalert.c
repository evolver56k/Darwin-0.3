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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/ioctl_compat.h>
#include <errno.h>
#include <paths.h>
#include <err.h>

#define KERNEL_PRIVATE
#include <dev/kmreg_com.h>
#undef KERNEL_PRIVATE

void alert_open ();
void alert_message (char* aMessage);
void alert_close ();

/**
 * Alert window controls
 **/

static int console;
static struct sgttyb console_sg;

static enum
{
    ALERT_FRESH,
    ALERT_OPENED,
    ALERT_PRINTED,
    ALERT_CLOSED
}
alert_state = ALERT_CLOSED;

void alert_open ()
{
    struct sgttyb sg;

    if (alert_state != ALERT_FRESH) return;

    /* Open console */
    if ((console = open(_PATH_CONSOLE, (O_RDWR|O_ALERT), 0)) < 0)
      {
        err(1, "Failed to open console");
      }

    /* Flush any existing input */
    ioctl(console, TIOCFLUSH, FREAD);

    /* Put console in CBREAK mode */
    if (ioctl(console, TIOCGETP, &sg) == -1)
      {
        err(1, "console TIOCGETP failed");
        close(console);
      }

    console_sg = sg;

    sg.sg_flags |= CBREAK;
    sg.sg_flags &= ~ECHO;

    if (ioctl(console, TIOCSETP, &sg) == -1)
      {
        err(1, "console TIOCSETP failed");
        close(console);
      }

    alert_state = ALERT_OPENED;    
}

void alert_message (char* aMessage)
{
    write(console, aMessage, strlen(aMessage));

    alert_state = ALERT_PRINTED;
}

void alert_close ()
{
    if (alert_state != ALERT_PRINTED) return;

#if 0
    fcntl(console, F_SETFL     ,  console_flags);
    ioctl(console, TIOCSETP    , &console_flags);
#endif
    ioctl(console, KMIOCRESTORE,  0            );

    close(console);

    alert_state = ALERT_CLOSED;
}

/**
 * Main
 **/

void usage ()
{
    printf("usage: fbalert replies message\n");
    exit(1);
}

int main (int argc, char* argv[])
{
    int  i = 1;
    char buf[] = "\0";
    char *allowed_replies = NULL;

    if (argc < 3) usage();

    allowed_replies = argv[1];

    alert_state = ALERT_FRESH;

    alert_open();

    while (++i < argc)
      {
        alert_message(argv[i]);
        alert_message("\n");
      }

    while (1)
      {
        if (read(console, (char*)&buf, 1) < 0)
            err(1, "Error while reading from console");

        if (strchr(allowed_replies, buf[0]))
            break;
      }

    alert_close();

    printf("%c\n", buf[0]);

    exit(0);
}
