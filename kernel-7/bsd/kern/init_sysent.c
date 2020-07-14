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

/* Copyright (c) 1995-98 Apple Computer, Inc. All Rights Reserved */
/* 
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 * HISTORY
 *
 *  22-Jan-98 Clark Warner (warner_c) at Apple
 *	Created new system calls for supporting HFS/HFS Plus file system semantics
 *
 *  04-Jun-95  Mac Gillon (mgillon) at NeXT
 *	Created new version based on NS3.3 and 4.4BSD
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>

/*
 * definitions
 */
int	nosys();
int	exit();
int	fork();
int	read();
int	write();
int	open();
int	close();
int	wait4();
int	link();
int	unlink();
int	chdir();
int	fchdir();
int	mknod();
int	chmod();
int	chown();
int	obreak();
int	getfsstat();
int	getpid();
int	mount();
int	unmount();
int	setuid();
int	getuid();
int	geteuid();
int	ptrace();
int	recvmsg();
int	sendmsg();
int	recvfrom();
int	accept();
int	getpeername();
int	getsockname();
int	access();
int	chflags();
int	fchflags();
int	sync();
int	kill();
int	getppid();
int	dup();
int	pipe();
int	getegid();
int	profil();
#if KTRACE
int	ktrace();
#else
#endif
int	sigaction();
int	getgid();
int	sigprocmask();
int	getlogin();
int	setlogin();
int	acct();
int	sigpending();
int	sigaltstack();
int	ioctl();
int	reboot();
int	revoke();
int	symlink();
int	readlink();
int	execve();
int	umask();
int	chroot();
/* int	msync(); */
int	vfork();
int	sbrk();
int	sstk();
int	ovadvise();
int	munmap();
int	mprotect();
int	madvise();
int	mincore();
int	getgroups();
int	setgroups();
int	getpgrp();
int	setpgid();
int	setitimer();
int	swapon();
int	getitimer();
int	getdtablesize();
int	dup2();
int	fcntl();
int	select();
int	fsync();
int	setpriority();
int	socket();
int	connect();
int	getpriority();
int	sigreturn();
int	bind();
int	setsockopt();
int	listen();
int	sigsuspend();
#if TRACE
int	vtrace();
#else
#endif
int	gettimeofday();
int	getrusage();
int	getsockopt();
int	readv();
int	writev();
int	settimeofday();
int	fchown();
int	fchmod();
int	rename();
int	flock();
int	mkfifo();
int	sendto();
int	shutdown();
int	socketpair();
int	mkdir();
int	rmdir();
int	utimes();
int	adjtime();
int	setsid();
int	quotactl();
int	nfssvc();
int	statfs();
int	fstatfs();
int	getfh();
int	setgid();
int	setegid();
int	seteuid();
#if LFS
int	lfs_bmapv();
int	lfs_markv();
int	lfs_segclean();
int	lfs_segwait();
#else
#endif
int	stat();
int	fstat();
int	lstat();
int	pathconf();
int	fpathconf();
int	getrlimit();
int	setrlimit();
int	getdirentries();
int	mmap();
int	nosys();
int	lseek();
int	truncate();
int	ftruncate();
int	__sysctl();
int	undelete();
int	mlock();
int	munlock();
int setprivexec();
int add_profil();
int table();
#if KDEBUG
int	syscall_kdebug();
#else
#endif

#if COMPAT_43
#define compat(name,n) syss(__CONCAT(o,name),n)
#define compatp(name,n) sysp(__CONCAT(o,name),n)

int	ocreat();
int	olseek();
int	ostat();
int	olstat();
#if KTRACE
#else
#endif
int	ofstat();
int	ogetkerninfo();
int	ogetdtablesize();
int	osmmap();
int	ogetpagesize();
int	ommap();
int	owait();
int	ogethostname();
int	osethostname();
int	oaccept();
int	osend();
int	orecv();
int	osigvec();
int	osigblock();
int	osigsetmask();
int	osigstack();
int	orecvmsg();
int	osendmsg();
#if TRACE
#else
#endif
int	orecvfrom();
int	osetreuid();
int	osetregid();
int	otruncate();
int	oftruncate();
int	ogetpeername();
int	ogethostid();
int	osethostid();
int	ogetrlimit();
int	osetrlimit();
int	okillpg();
int	oquota();
int	ogetsockname();
int ogetdomainname();
int osetdomainname();
int	owait3();
#if NFS
#else
#endif
int	ogetdirentries();
#if NFS
#else
#endif
#if LFS
#else
#endif

#if NETAT
int ATsocket();
int ATgetmsg();
int ATputmsg();
int ATPsndreq();
int ATPsndrsp();
int ATPgetreq();
int ATPgetrsp();
#endif /* NETAT */

/* Calls for supporting HFS Semantics */

int mkcomplex();		
int statv();				
int lstatv();				
int fstatv();			
int getattrlist();		
int setattrlist();		
int getdirentryattr();		
int exchangedata();		
int checkuseraccess();		
int searchfs();
	
/* end of HFS calls */

#else /* COMPAT_43 */
#define compat(n, name) syss(nosys,0)
#define compatp(n, name) sysp(nosys,0)
#endif /* COMPAT_43 */

int watchevent();
int waitevent();
int modwatch();

/*
 * System call switch table.
 */
 
/* serial or parallel system call */
#define syss(fn,no) {no, 0, fn}
#define sysp(fn,no) {no, 1, fn}

struct sysent sysent[] = {
	syss(nosys,0),			/*   0 = indir */
	syss(exit,1),			/*   1 = exit */
	syss(fork,0),			/*   2 = fork */
	sysp(read,3),			/*   3 = read */
	sysp(write,3),			/*   4 = write */
	syss(open,3),			/*   5 = open */
	syss(close,1),			/*   6 = close */
	syss(wait4, 4),			/*   7 = wait4 */
	compat(creat,2),	/*   8 = old creat */
	syss(link,2),			/*   9 = link */
	syss(unlink,1),			/*  10 = unlink */
	syss(nosys, 0),			/*  11 was obsolete execv */
	syss(chdir,1),			/*  12 = chdir */
	syss(fchdir,1),			/*  13 = fchdir */
	syss(mknod,3),			/*  14 = mknod */
	syss(chmod,2),			/*  15 = chmod */
	syss(chown,3),			/*  16 = chown; now 3 args */
	syss(obreak,1),			/*  17 = old break */
	syss(getfsstat, 3),		/*  18 = getfsstat */
	compat(lseek,3),	/*  19 = old lseek */
	sysp(getpid,0),			/*  20 = getpid */
	syss(nosys, 0),			/*  21 was obsolete mount */
	syss(nosys, 0),			/*  22 was obsolete umount */
	syss(setuid,1),			/*  23 = setuid */
	sysp(getuid,0),			/*  24 = getuid */
	sysp(geteuid,0),		/*  25 = geteuid */
	syss(ptrace,4),			/*  26 = ptrace */
	syss(recvmsg,3),		/*  27 = recvmsg */
	syss(sendmsg,3),		/*  28 = sendmsg */
	syss(recvfrom,6),		/*  29 = recvfrom */
	syss(accept,3),			/*  30 = accept */
	syss(getpeername,3),	/*  31 = getpeername */
	syss(getsockname,3),	/*  32 = getsockname */
	syss(access,2),			/*  33 = access */
	syss(chflags,2),		/* 34 = chflags */
	syss(fchflags,2),		/* 35 = fchflags */
	syss(sync,0),			/*  36 = sync */
	syss(kill,2),			/*  37 = kill */
	compat(stat,2),	/*  38 = old stat */
	sysp(getppid,0),		/*  39 = getppid */
	compat(lstat,2),	/*  40 = old lstat */
	syss(dup,2),			/*  41 = dup */
	syss(pipe,0),			/*  42 = pipe */
	sysp(getegid,0),		/*  43 = getegid */
	syss(profil,4),			/*  44 = profil */
#if KTRACE
	syss(ktrace,4),			/*  45 = ktrace */
#else
	syss(nosys,0),			/*  45 = nosys */
#endif
	syss(sigaction,3),		/*  46 = sigaction */
	sysp(getgid,0),			/*  47 = getgid */
	syss(sigprocmask,2),	/*  48 = sigprocmask */
	syss(getlogin,2),		/*  49 = getlogin */
	syss(setlogin,1),		/*  50 = setlogin */
	syss(acct,1),			/*  51 = turn acct off/on */
	syss(sigpending,0),		/*  52 = sigpending */
	syss(sigaltstack,2),	/*  53 = sigaltstack */
	syss(ioctl,3),			/*  54 = ioctl */
	syss(reboot,2),			/*  55 = reboot */
	syss(revoke,1),			/*  56 = revoke */
	syss(symlink,2),		/*  57 = symlink */
	syss(readlink,3),		/*  58 = readlink */
	syss(execve,3),			/*  59 = execve */
	syss(umask,1),			/*  60 = umask */
	syss(chroot,1),			/*  61 = chroot */
	compat(fstat,2),	/*  62 = old fstat */
	syss(nosys,0),			/*  63 = used internally, reserved */
	compat(getpagesize,0),	/*  64 = old getpagesize */
//	syss(msync,5),			/*  65 = msync */
#warning stubbed out msync
	syss(nosys, 0),
	syss(vfork,0),			/*  66 = vfork */
	syss(nosys,0),			/*  67 was obsolete vread */
	syss(nosys,0),			/*  68 was obsolete vwrite */
	syss(sbrk,1),			/*  69 = sbrk */
	syss(sstk,1),			/*  70 = sstk */
	compat(smmap,6),		/*  71 = old mmap */
	syss(ovadvise,1),		/*  72 = old vadvise */
	syss(munmap,2),			/*  73 = munmap */
	syss(mprotect,3),		/*  74 = mprotect */
	syss(madvise,3),		/*  75 = madvise */
	syss(nosys,0),			/*  76 was obsolete vhangup */
	syss(nosys,0),			/*  77 was obsolete vlimit */
	syss(mincore,3),		/*  78 = mincore */
	sysp(getgroups,2),		/*  79 = getgroups */
	sysp(setgroups,2),		/*  80 = setgroups */
	sysp(getpgrp,0),		/*  81 = getpgrp */
	sysp(setpgid,2),		/*  82 = setpgid */
	syss(setitimer,3),		/*  83 = setitimer */
	compat(wait,0),	/*  84 = old wait */
	syss(swapon,1),			/*  85 = swapon */
	syss(getitimer,2),		/*  86 = getitimer */
	compat(gethostname,2),	/*  87 = old gethostname */
	compat(sethostname,2),	/*  88 = old sethostname */
	compatp(getdtablesize,0),	/*  89 = old getdtablesize */
	syss(dup2,2),			/*  90 = dup2 */
	syss(nosys,0),			/*  91 was obsolete getdopt */
	syss(fcntl,3),			/*  92 = fcntl */
	syss(select,5),			/*  93 = select */
	syss(nosys,0),			/*  94 was obsolete setdopt */
	syss(fsync,1),			/*  95 = fsync */
	sysp(setpriority,3),	/*  96 = setpriority */
	syss(socket,3),			/*  97 = socket */
	syss(connect,3),		/*  98 = connect */
	compat(accept,3),	/*  99 = accept */
	sysp(getpriority,2),	/* 100 = getpriority */
	compat(send,4),		/* 101 = old send */
	compat(recv,4),		/* 102 = old recv */
	syss(sigreturn,1),		/* 103 = sigreturn */
	syss(bind,3),			/* 104 = bind */
	syss(setsockopt,5),		/* 105 = setsockopt */
	syss(listen,2),			/* 106 = listen */
	syss(nosys,0),			/* 107 was vtimes */
	compat(sigvec,3),		/* 108 = sigvec */
	compat(sigblock,1),		/* 109 = sigblock */
	compat(sigsetmask,1),	/* 110 = sigsetmask */
	syss(sigsuspend,1),		/* 111 = sigpause */
	compat(sigstack,2),	/* 112 = sigstack */
	compat(recvmsg,3),	/* 113 = recvmsg */
	compat(sendmsg,3),	/* 114 = sendmsg */
	syss(nosys,0),			/* 115 = old vtrace */
	syss(gettimeofday,2),		/* 116 = gettimeofday */
	sysp(getrusage,2),		/* 117 = getrusage */
	syss(getsockopt,5),		/* 118 = getsockopt */
	syss(nosys,0),			/* 119 = old resuba */
	sysp(readv,3),			/* 120 = readv */
	sysp(writev,3),			/* 121 = writev */
	syss(settimeofday,2),	/* 122 = settimeofday */
	syss(fchown,3),			/* 123 = fchown */
	syss(fchmod,2),			/* 124 = fchmod */
	compat(recvfrom,6),	/* 125 = recvfrom */
	compat(setreuid,2),	/* 126 = setreuid */
	compat(setregid,2),	/* 127 = setregid */
	syss(rename,2),			/* 128 = rename */
	compat(truncate,2),	/* 129 = old truncate */
	compat(ftruncate,2),	/* 130 = ftruncate */
	syss(flock,2),			/* 131 = flock */
	syss(mkfifo,2),			/* 132 = nosys */
	syss(sendto,6),			/* 133 = sendto */
	syss(shutdown,2),		/* 134 = shutdown */
	syss(socketpair,5),		/* 135 = socketpair */
	syss(mkdir,2),			/* 136 = mkdir */
	syss(rmdir,1),			/* 137 = rmdir */
	syss(utimes,2),			/* 138 = utimes */
	syss(nosys,0),			/* 139 = used internally */
	syss(adjtime,2),		/* 140 = adjtime */
	compat(getpeername,3),/* 141 = getpeername */
	compat(gethostid,0),	/* 142 = old gethostid */
	sysp(nosys,0),			/* 143 = old sethostid */
	compat(getrlimit,2),		/* 144 = old getrlimit */
	compat(setrlimit,2),		/* 145 = old setrlimit */
	compat(killpg,2),	/* 146 = old killpg */
	syss(setsid,0),			/* 147 = setsid */
	syss(nosys,0),			/* 148 was setquota */
	syss(nosys,0),			/* 149 was qquota */
	compat(getsockname,3),/* 150 = getsockname */
	/*
	 * Syscalls 151-183 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */
	syss(nosys,0),		/* 151 was m68k specific machparam */
	sysp(setprivexec,1),/* 152 = setprivexec */
	syss(nosys,0),		/* 153 */
	syss(nosys,0),		/* 154 */
	syss(nfssvc,2),			/* 155 = nfs_svc */
	compat(getdirentries,4),	/* 156 = old getdirentries */
	syss(statfs, 2),		/* 157 = statfs */
	syss(fstatfs, 2),		/* 158 = fstatfs */
	syss(unmount, 2),		/* 159 = unmount */
	syss(nosys,0),			/* 160 was async_daemon */
	syss(getfh,2),			/* 161 = get file handle */
	compat(getdomainname,2),	/* 162 = getdomainname */
	compat(setdomainname,2),	/* 163 = setdomainname */
	syss(nosys,0),			/* 164 */
#if	QUOTA
	syss(quotactl, 4),		/* 165 = quotactl */
#else	QUOTA
	syss(nosys, 0),		/* 165 = not configured */
#endif	/* QUOTA */
	syss(nosys,0),			/* 166 was exportfs */
	syss(mount, 4),			/* 167 = mount */
	syss(nosys,0),			/* 168 was ustat */
	syss(nosys,0),		    /* 169 = nosys */
	syss(table,5),			/* 170 = table */
	compat(wait3,3),	/* 171 = old wait3 */
	syss(nosys,0),			/* 172 was rpause */
	syss(nosys,0),			/* 173 = nosys */
	syss(nosys,0),			/* 174 was getdents */
	syss(nosys,0),			/* 175 was gc_control */
	syss(add_profil,4),		/* 176 = add_profil */
	syss(nosys,0),			/* 177 */
	syss(nosys,0),			/* 178 */
	syss(nosys,0),			/* 179 */
#if KDEBUG
	syss(syscall_kdebug,0),		/* 180 */
#else
	syss(nosys,0),			/* 180 */
#endif
	syss(setgid,1),			/* 181 */
	syss(setegid,1),		/* 182 */
	syss(seteuid,1),			/* 183 */
#if LFS
	syss(lfs_bmapv,3),		/* 184 = lfs_bmapv */
	syss(lfs_markv,3),		/* 185 = lfs_markv */
	syss(lfs_segclean,2),	/* 186 = lfs_segclean */
	syss(lfs_segwait,2),	/* 187 = lfs_segwait */
#else
	syss(nosys,0),			/* 184 = nosys */
	syss(nosys,0),			/* 185 = nosys */
	syss(nosys,0),			/* 186 = nosys */
	syss(nosys,0),			/* 187 = nosys */
#endif
	syss(stat,2),			/* 188 = stat */
	syss(fstat,2),			/* 189 = fstat */
	syss(lstat,2),			/* 190 = lstat */
	syss(pathconf,2),		/* 191 = pathconf */
	syss(fpathconf,2),		/* 192 = fpathconf */
	syss(nosys,0),			/* 193 = nosys */
	syss(getrlimit,2),		/* 194 = getrlimit */
	syss(setrlimit,2),		/* 195 = setrlimit */
	syss(getdirentries,4),	/* 196 = getdirentries */
#ifdef DOUBLE_ALIGN_PARAMS
	syss(mmap,8),			/* 197 = mmap */
#else
	syss(mmap,7),			/* 197 = mmap */
#endif
	syss(nosys,0),			/* 198 = __syscall */
	syss(lseek,5),			/* 199 = lseek */
	syss(truncate,4),		/* 200 = truncate */
	syss(ftruncate,4),		/* 201 = ftruncate */
	syss(__sysctl,6),		/* 202 = __sysctl */
	sysp(getdtablesize, 0),		/* 203 getdtablesize */
	syss(nosys, 0),			/* 204 unused */
#if NETAT
	syss(undelete,1),		/* 205 = undelete */
	syss(ATsocket,1),		/* 206 = AppleTalk ATsocket */
	syss(ATgetmsg,4),		/* 207 = AppleTalk ATgetmsg*/
	syss(ATputmsg,4),		/* 208 = AppleTalk ATputmsg*/
	syss(ATPsndreq,4),		/* 209 = AppleTalk ATPsndreq*/
	syss(ATPsndrsp,4),		/* 210 = AppleTalk ATPsndrsp*/
	syss(ATPgetreq,3),		/* 211 = AppleTalk ATPgetreq*/
	syss(ATPgetrsp,2),		/* 212 = AppleTalk ATPgetrsp*/
	syss(nosys,0),			/* 213 = Reserved for AT expansion */
	syss(nosys,0),			/* 214 = Reserved for AT expansion */
	syss(nosys,0),			/* 215 = Reserved for AT expansion */
#else
	syss(undelete,1),		/* 205 = undelete */

/*  System calls 205 - 215 are reserved to allow HFS and AT to coexist */
/*  CHW 1/22/98							       */

	syss(nosys,0),			/* 206 = Reserved for AppleTalk */
	syss(nosys,0),			/* 207 = Reserved for AppleTalk */
	syss(nosys,0),			/* 208 = Reserved for AppleTalk */
	syss(nosys,0),			/* 209 = Reserved for AppleTalk */
	syss(nosys,0),			/* 210 = Reserved for AppleTalk */
	syss(nosys,0),			/* 211 = Reserved for AppleTalk */
	syss(nosys,0),			/* 212 = Reserved for AppleTalk */
	syss(nosys,0),			/* 213 = Reserved for AppleTalk */
	syss(nosys,0),			/* 214 = Reserved for AppleTalk */
	syss(nosys,0),			/* 215 = Reserved for AppleTalk */
#endif /* NETAT */

/*
 * System Calls 216 - 230 are reserved for calls to support HFS/HFS Plus
 * file system semantics. Currently, we only use 215-225.  The rest is 
 * for future expansion in anticipation of new MacOS APIs for HFS Plus.
 * These calls are not conditionalized becuase while they are specific
 * to HFS semantics, they are not specific to the HFS filesystem.
 * We expect all filesystems to recognize the call and report that it is
 * not supported or to actually implement it.
 */
	syss(mkcomplex,3),	/* 216 = HFS make complex file call (multipel forks */
	syss(statv,2),		/* 217 = HFS statv extended stat call for HFS */
	syss(lstatv,2),		/* 218 = HFS lstatv extended lstat call for HFS */	
	syss(fstatv,2),		/* 219 = HFS fstatv extended fstat call for HFS */
	syss(getattrlist,3),	/* 220 = HFS getarrtlist get attribute list cal */
	syss(setattrlist,3),	/* 221 = HFS setattrlist set attribute list */
	syss(getdirentryattr,4),	/* 222 = HFS getdirenttryattr get directory attributes */
	syss(exchangedata,2),	/* 223 = HFS exchangedata exchange file contents */
	syss(checkuseraccess,5),	/* 224 = HFS checkuseraccess check access to a file */
	syss(searchfs,6),	/* 225 = HFS searchfs to implement catalog searching */
	syss(nosys,0),		/* 226 */
	syss(nosys,0),		/* 227 */
	syss(nosys,0),		/* 228 */
	syss(nosys,0),		/* 229 */
	syss(nosys,0),		/* 230 */
	syss(watchevent,2),		/* 231 */
	syss(waitevent,2),		/* 232 */
	syss(modwatch,2)		/* 233 */
};
int	nsysent = sizeof(sysent) / sizeof(sysent[0]);
