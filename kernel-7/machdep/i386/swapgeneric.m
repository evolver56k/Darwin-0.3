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
 * Copyright (c) 1987, 1988 NeXT, Inc.
 *
 * HISTORY
 *
 * Tue Dec  7 17:37:14 PST 1993 Matt Watson (mwatson) at NeXT
 *	Added netboot capability.
 *
 * 07-Apr-90	Doug Mitchell at NeXT
 *	Added fd driver support.
 *
 * 16-Feb-88  John Seamons (jks) at NeXT
 *	Updated to Mach release 2.
 *
 * 29-Oct-87  Robert V. Baron (rvb) at Carnegie-Mellon University
 *	Allow root to be an arbitrary slice on the drive.
 *
 * 14-Nov-86  John Seamons (jks) at NeXT
 *	Ported to NeXT.
 */ 

#import <sys/param.h>
#import <sys/conf.h>
#import <sys/buf.h>
#import <sys/time.h>
#import <sys/systm.h>
#import <sys/mount.h>
#import <sys/reboot.h>
#import <sys/vnode.h>
#import <sys/socket.h>
#import <sys/errno.h>
#import <sys/ioctl.h>
#import <net/if.h>
#import <netinet/in_systm.h>
#import <netinet/in.h>
#import <netinet/in_var.h>
#import <netinet/ip.h>
#import <netinet/udp.h>
#import <netinet/if_ether.h>
#import <machdep/i386/kernBootStruct.h>
#import <driverkit/kernelDriver.h>
#import <bsd/dev/scsireg.h>
/*
 * Hack to avoid collision with vnode.h!
 */
#undef IO_UNIT
#import <driverkit/IODeviceParams.h>
#import <driverkit/SCSIDisk.h>

extern int ffs_mountroot();

#ifdef	NFSCLIENT
extern char	*nfsbootdevname;	/* nfs_boot.c */
extern int nfs_mountroot(); 	/* nfs_vfsops.c */
#else	/* NFSCLIENT */
static char	*nfsbootdevname;
static int nfs_mountroot() { return ENOSYS; }
#endif	/* NFSCLIENT */

extern int (*mountroot)();

extern char hostname[], boot_info[], boot_dev[];
extern char domainname[];
extern struct xdr_ops xdrmbuf_ops;
extern int hostnamelen;
char rootdevice[8];

int	boottype = 0, boothowto = RB_DEBUG;

/*
 * Generic configuration;  all in one
 */
extern long	dumplo;
int	dmmin, dmmax, dmtext;

struct	genericconf {
	char	*gc_name;
	dev_t	gc_root;	/* NODEV means remote root */
} genericconf[] = {
	{ "sd", makedev(6, 0) },
	{ "hd", makedev(3, 0) },
	{ "fd", makedev(1, 0) },
	{ "en", NODEV },
	{ "tr", NODEV },
	{ 0 },
};

static struct genericconf *gc;
static int scsiControllerDetected(void);

setconf()
{
	extern char	init_args[];
	int		devmajor, devminor;
	int unit=0;
	int slice = 0;
	char *name, root_name[128];
	KERNBOOTSTRUCT *kernBootStruct = (KERNBOOTSTRUCT *)KERNSTRUCT_ADDR;
	char scsiName[10];
	int panelUp = 0;

	if (kernBootStruct->magicCookie != KERNBOOTMAGIC) 
		panic("invalid boot struct");
	
	if ((boothowto & RB_ASKNAME) || rootdevice[0]) {
retry:
		if (boothowto & RB_ASKNAME) {
			if( 0 == panelUp) {
                            DoSafeAlert("Root Device?", "", TRUE);
			    panelUp = 1;
			}
			printf("root device? ");
			name = root_name;
			gets(name, name);
		} else {
			/* 
			 * If we've been asked for CDROM, find the 
			 * corresponding sd device.
			 */
			if(!strcmp("cdrom", rootdevice)) {
			    id scsiId = nil;
			    int i;
			    IOReturn rtn;
			    unsigned char inquiryType;
			    
			    for(i=0; i<16; i++) {
			    	sprintf(scsiName, "sd%d", i);
				rtn = IOGetObjectForDeviceName(scsiName,
					&scsiId);
				if(rtn) {
				    break;	// no more sd's
				}
				inquiryType = [scsiId inquiryDeviceType];
				if(inquiryType == DEVTYPE_CDROM) {
				    name = scsiName;
				    goto name_found;
				}
				/* else continue to next sd */
			    }
			    if (scsiControllerDetected())
				printf("No CD-ROM drive found\n");
			    else
				printf("No SCSI controller or CD-ROM drive found\n");
			    goto bad;
			}
			else {
			    name = rootdevice;
			}
name_found:
			printf ("root on %s\n", name);
		}
		for (gc = genericconf; gc->gc_name; gc++)
			if (gc->gc_name[0] == name[0] &&
			    gc->gc_name[1] == name[1])
				goto gotit;
		goto bad;

gotit:
		if (gc->gc_root == NODEV) {
			goto found;
		}
		if (name[2] >= '0' && name[2] <= '7') {
			if (name[3] >= 'a' && name[3] <= 'h') {
				slice = name[3] - 'a';
			} else if (name[3]) {
				printf("bad partition number\n");
				goto bad;
			}
			unit = name[2] - '0';
			goto found;
		}
		printf("bad/missing unit number\n");
bad:
		for (gc = genericconf; gc->gc_name; gc++)
			printf("%s%s%%d",
			       (gc == genericconf)?"use ":
				    (((gc+1)->gc_name)?", ":" or "),
			       gc->gc_name);
		printf("\n");
		boothowto |= RB_ASKNAME;
		goto retry;
found:
	       if (gc->gc_root == NODEV) {
				printf("mounting nfs root\n");
				nfsbootdevname = rootdevice;
				mountroot = nfs_mountroot;
				rootdev = NODEV;
	       } else {
				mountroot = ffs_mountroot;
				gc->gc_root = makedev(major(gc->gc_root), unit*8+slice);
				rootdev = gc->gc_root;
	       }
	}
	else
	{
		boottype = kernBootStruct->rootdev;
		devmajor = (boottype >> B_TYPESHIFT) & B_TYPEMASK;
		devminor = (boottype >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
		devminor |= (((boottype >> B_UNITSHIFT) & B_UNITMASK) << 3);

		rootdev = makedev(devmajor, devminor);
	}
		printf("rootdev %x, howto %x\n", rootdev, boothowto);

    if( panelUp)
	alert_done();
}

gets(cp, lp)
	char *cp, *lp;
{
	register c;

	for (;;) {
		c = cngetc() & 0177;
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\177':
			if (lp == cp) {
				cnputc('\b');
				continue;
			}
			cnputc('\b');
			cnputc('\b');
			/* fall into ... */
		case '\b':
			if (lp == cp) {
				cnputc('\b');
				continue;
			}
			cnputc(' ');
			cnputc('\b');
			lp--;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}

/*
 * If booted with ASKNAME, prompt on the console for a filesystem
 * name and return it.
 */
getfsname(askfor, name)
	char *askfor;
	char *name;
{

	if (boothowto & RB_ASKNAME) {
		printf("%s key [%s]: ", askfor, askfor);
		gets(name, name);
	}
}

static int
scsiControllerDetected(void)
{
    IOReturn rtn;
    IOObjectNumber i;
    IOString name, kind;
    Protocol **protos;
    id deviceId;
    int j;
    
    protos = [SCSIDisk requiredProtocols];
    if (protos == NULL || *protos == nil) {
	/*
	 * We can't search for a SCSI adapter,
	 * so don't say we didn't find one.
	 */
	return 1;
    }
    for(i = 0; ; i++) {
	rtn = [IODevice lookupByObjectNumber:i deviceKind:&kind 
		deviceName:&name];
	if (rtn != IO_R_SUCCESS && rtn != IO_R_OFFLINE)
	    break;
	rtn = IOGetObjectForDeviceName(name, &deviceId);
	if (rtn == IO_R_SUCCESS) {
	    int conforms = 1;
	    /* Check to see if it implements the correct protocols. */
	    for(j=0; protos[j]; j++) {
		    if(![deviceId conformsTo:protos[j]]) {
			conforms = 0;
			break;
		    }
	    }
	    if (conforms)
		return 1;
	}
    }
    return 0;
}

#if 0
int
initrootnet()
{
   struct socket *so = NULL;
   struct ifreq ifr;
   int error;
   extern struct in_ifaddr *in_ifaddr;
   int message = 0;

   /* Is there already a primary network? */
   if (in_ifaddr && !(in_ifaddr->ia_ifp->if_flags & IFF_LOOPBACK))
      return(0);

   ifr.ifr_name[0] = gc->gc_name[0];
   ifr.ifr_name[1] = gc->gc_name[1];
   ifr.ifr_name[2] = '0';
   ifr.ifr_name[3] = '\0';
   error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
   if (error) {
	    printf("initrootnet: socreate failed\n");
	    goto errout;
   }
   ifr.ifr_addr.sa_family = AF_INET;
   while (error = ifioctl(so, SIOCAUTOADDR, (caddr_t)&ifr)) {
      if (error == ETIMEDOUT) {
	 if (!message) {
	    printf("initrootnet: BOOTP timed out, still trying...\n");
	    message++;
	 }
      } else {
	 printf("initrootnet: autoaddr failed\n");
	 goto errout;
      }
   }
   if (message) {
      printf("initrootnet: BOOTP [OK].\n");
   }
   printf("primary network interface: %s [%s]\n", ifr.ifr_name,
      inet_ntoa(&((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
errout:	
   if (so) {
      soclose(so);
   }
   return(error);
}
#endif /* 0 */
