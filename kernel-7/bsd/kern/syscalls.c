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

/* Copyright (c) 1992,1995-98 Apple Computer, Inc.  All rights resereved. */
/*
 *
 * The NEXTSTEP Software License Agreement specifies the terms
 * and conditions for redistribution.
 *
 */
/*
 * HISTORY
 *
 * 01-22-98 Clark Warner (warner_c) at Apple
 *    	Created new HFS style Systemcalls
 *
 * 25-May-95 Mac Gillon (mgillon) at NeXT
 *	Created from NS 3.3 and 4.4BSD
 *
 */

char *syscallnames[] = {
	"syscall",			/* 0 = syscall */
	"exit",				/* 1 = exit */
	"fork",				/* 2 = fork */
	"read",				/* 3 = read */
	"write",			/* 4 = write */
	"open",				/* 5 = open */
	"close",			/* 6 = close */
	"wait4",			/* 7 = wait4 */
	"old_creat",		/* 8 = old creat */
	"link",				/* 9 = link */
	"unlink",			/* 10 = unlink */
	"obs_execv",		/* 11 = obsolete execv */
	"chdir",			/* 12 = chdir */
	"fchdir",			/* 13 = fchdir */
	"mknod",			/* 14 = mknod */
	"chmod",			/* 15 = chmod */
	"chown",			/* 16 = chown */
	"sbreak",			/* 17 = obsolete sbreak */
	"obs_stat",			/* 18 = obsolete stat */
	"old_lseek",		/* 19 = old lseek */
	"getpid",			/* 20 = getpid */
	"obs_mount",		/* 21 = obsolete mount */
	"obs_unmount",		/* 22 = obsolete unmount */
	"setuid",			/* 23 = setuid */
	"getuid",			/* 24 = getuid */
	"geteuid",			/* 25 = geteuid */
	"ptrace",			/* 26 = ptrace */
	"recvmsg",			/* 27 = recvmsg */
	"sendmsg",			/* 28 = sendmsg */
	"recvfrom",			/* 29 = recvfrom */
	"accept",			/* 30 = accept */
	"getpeername",		/* 31 = getpeername */
	"getsockname",		/* 32 = getsockname */
	"access",			/* 33 = access */
	"chflags",			/* 34 = chflags */
	"fchflags",			/* 35 = fchflags */
	"sync",				/* 36 = sync */
	"kill",				/* 37 = kill */
	"old_stat",			/* 38 = old stat */
	"getppid",			/* 39 = getppid */
	"old_lstat",		/* 40 = old lstat */
	"dup",				/* 41 = dup */
	"pipe",				/* 42 = pipe */
	"getegid",			/* 43 = getegid */
	"profil",			/* 44 = profil */
	"ktrace",			/* 45 = ktrace */
	"sigaction",		/* 46 = sigaction */
	"getgid",			/* 47 = getgid */
	"sigprocmask",		/* 48 = sigprocmask */
	"getlogin",			/* 49 = getlogin */
	"setlogin",			/* 50 = setlogin */
	"acct",				/* 51 = acct */
	"sigpending",		/* 52 = sigpending */
	"sigaltstack",		/* 53 = sigaltstack */
	"ioctl",			/* 54 = ioctl */
	"reboot",			/* 55 = reboot */
	"revoke",			/* 56 = revoke */
	"symlink",			/* 57 = symlink */
	"readlink",			/* 58 = readlink */
	"execve",			/* 59 = execve */
	"umask",			/* 60 = umask */
	"chroot",			/* 61 = chroot */
	"old_fstat",		/* 62 = old fstat */
	"old_getkerninfo",	/* 63 = old getkerninfo */
	"old_getpagesize",	/* 64 = old getpagesize */
	"msync",			/* 65 = msync */
	"vfork",			/* 66 = vfork */
	"obs_vread",		/* 67 = obsolete vread */
	"obs_vwrite",		/* 68 = obsolete vwrite */
	"sbrk",				/* 69 = sbrk */
	"sstk",				/* 70 = sstk */
	"old_mmap",			/* 71 = old mmap */
	"obs_vadvise",		/* 72 = obsolete vadvise */
	"munmap",			/* 73 = munmap */
	"mprotect",			/* 74 = mprotect */
	"madvise",			/* 75 = madvise */
	"obs_vhangup",		/* 76 = obsolete vhangup */
	"obs_vlimit",		/* 77 = obsolete vlimit */
	"mincore",			/* 78 = mincore */
	"getgroups",		/* 79 = getgroups */
	"setgroups",		/* 80 = setgroups */
	"getpgrp",			/* 81 = getpgrp */
	"setpgid",			/* 82 = setpgid */
	"setitimer",		/* 83 = setitimer */
	"old_wait",			/* 84 = old wait */
	"swapon",			/* 85 = swapon */
	"getitimer",		/* 86 = getitimer */
	"old_gethostname",	/* 87 = old gethostname */
	"old_sethostname",	/* 88 = old sethostname */
	"getdtablesize",	/* 89 = getdtablesize */
	"dup2",				/* 90 = dup2 */
	"#91",				/* 91 = getdopt */
	"fcntl",			/* 92 = fcntl */
	"select",			/* 93 = select */
	"#94",				/* 94 = setdopt */
	"fsync",			/* 95 = fsync */
	"setpriority",		/* 96 = setpriority */
	"socket",			/* 97 = socket */
	"connect",			/* 98 = connect */
	"old_accept",		/* 99 = old accept */
	"getpriority",		/* 100 = getpriority */
	"old_send",			/* 101 = old send */
	"old_recv",			/* 102 = old recv */
	"sigreturn",		/* 103 = sigreturn */
	"bind",				/* 104 = bind */
	"setsockopt",		/* 105 = setsockopt */
	"listen",			/* 106 = listen */
	"obs_vtimes",		/* 107 = obsolete vtimes */
	"old_sigvec",		/* 108 = old sigvec */
	"old_sigblock",		/* 109 = old sigblock */
	"old_sigsetmask",	/* 110 = old sigsetmask */
	"sigsuspend",		/* 111 = sigsuspend */
	"old_sigstack",		/* 112 = old sigstack */
	"old_recvmsg",		/* 113 = old recvmsg */
	"old_sendmsg",		/* 114 = old sendmsg */
	"obs_vtrace",		/* 115 = obsolete vtrace */
	"gettimeofday",		/* 116 = gettimeofday */
	"getrusage",		/* 117 = getrusage */
	"getsockopt",		/* 118 = getsockopt */
	"#119",				/* 119 = nosys */
	"readv",			/* 120 = readv */
	"writev",			/* 121 = writev */
	"settimeofday",		/* 122 = settimeofday */
	"fchown",			/* 123 = fchown */
	"fchmod",			/* 124 = fchmod */
	"old_recvfrom",		/* 125 = old recvfrom */
	"old_setreuid",		/* 126 = old setreuid */
	"old_setregid",		/* 127 = old setregid */
	"rename",			/* 128 = rename */
	"old_truncate",		/* 129 = old truncate */
	"old_ftruncate",	/* 130 = old ftruncate */
	"flock",			/* 131 = flock */
	"mkfifo",			/* 132 = mkfifo */
	"sendto",			/* 133 = sendto */
	"shutdown",			/* 134 = shutdown */
	"socketpair",		/* 135 = socketpair */
	"mkdir",			/* 136 = mkdir */
	"rmdir",			/* 137 = rmdir */
	"utimes",			/* 138 = utimes */
	"#139",				/* 139 = nosys */
	"adjtime",			/* 140 = adjtime */
	"old_getpeername",	/* 141 = old getpeername */
	"old_gethostid",	/* 142 = old gethostid */
	"old_sethostid",	/* 143 = old sethostid */
	"old_getrlimit",	/* 144 = old getrlimit */
	"old_setrlimit",	/* 145 = old setrlimit */
	"old_killpg",		/* 146 = old killpg */
	"setsid",			/* 147 = setsid */
	"obs_setquota",		/* 148 = obsolete setquota */
	"obs_quota",		/* 149 = obsolete quota */
	"old_getsockname",	/* 150 = old getsockname */
	"#151",				/* 151 = nosys */
	"setprivexec",		/* 152 = setprivexec */
	"#153",				/* 153 = nosys */
	"#154",				/* 154 = nosys */
	"nfssvc",			/* 155 = nfssvc */
	"getdirentries",	/* 156 =getdirentries */
	"statfs",			/* 157 = statfs */
	"fstatfs",			/* 158 = fstatfs */
	"unmount",			/* 159 = unmount */
	"obs_async_daemon",	/* 160 = obsolete async_daemon */
	"getfh",			/* 161 = getfh */
	"old_getdomainname",/* 162 = old getdomainname */
	"old_setdomainname",/* 163 = old setdomainname */
	"obs_pcfs_mount",	/* 164 = obsolete pcfs_mount */
	"quotactl",			/* 165 = quotactl */
	"obs_exportfs",		/* 166 = obsolete exportfs */
	"mount",			/* 167 = mount */
	"obs_ustat",		/* 168 = obsolete ustat */
	"#169",				/* 169 = nosys */
	"obs_table",		/* 170 = obsolete table */
	"old_wait_3",		/* 171 = old wait_3 */
	"obs_rpause",		/* 172 = obsolete rpause */
	"#173",				/* 173 = nosys */
	"obs_getdents",		/* 174 = obsolete getdents */
	"#175",				/* 175 = nosys */
	"add_profil",		/* 176 = add_profil */ /* NeXT */
	"#177",				/* 177 = nosys */
	"#178",				/* 178 = nosys */
	"#179",				/* 179 = nosys */
	"#180",				/* 180 = nosys */
	"setgid",			/* 181 = setgid */
	"setegid",			/* 182 = setegid */
	"seteuid",			/* 183 = seteuid */
#ifdef LFS
	"lfs_bmapv",		/* 184 = lfs_bmapv */
	"lfs_markv",		/* 185 = lfs_markv */
	"lfs_segclean",		/* 186 = lfs_segclean */
	"lfs_segwait",		/* 187 = lfs_segwait */
#else
	"#184",				/* 184 = nosys */
	"#185",				/* 185 = nosys */
	"#186",				/* 186 = nosys */
	"#187",				/* 187 = nosys */
#endif
	"stat",				/* 188 = stat */
	"fstat",			/* 189 = fstat */
	"lstat",			/* 190 = lstat */
	"pathconf",			/* 191 = pathconf */
	"fpathconf",		/* 192 = fpathconf */
	"#193",				/* 193 = nosys */
	"getrlimit",		/* 194 = getrlimit */
	"setrlimit",		/* 195 = setrlimit */
	"#196",				/* 196 = unused */
	"mmap",				/* 197 = mmap */
	"__syscall",		/* 198 = __syscall */
	"lseek",			/* 199 = lseek */
	"truncate",			/* 200 = truncate */
	"ftruncate",		/* 201 = ftruncate */
	"__sysctl",			/* 202 = __sysctl */
	"mlock",			/* 203 = mlock */
	"munlock",			/* 204 = munlock */
	"#205",			/* 205 = nosys */

	/*
	 * 206 - 215 are all reserved for AppleTalk.
	 * When AppleTalk is defined some of them are in use
	 */

	"#206",			/* 206 = nosys */
	"#207",			/* 207 = nosys */
	"#208",			/* 208 = nosys */
	"#209",			/* 209 = nosys */
	"#210",			/* 210 = nosys */
	"#211",			/* 205 = nosys */
	"#212",			/* 206 = nosys */
	"#213",			/* 207 = nosys */
	"#214",			/* 208 = nosys */
	"#215",			/* 209 = nosys */
	"mkcomplex",		/* 216 = mkcomplex	*/
	"statv",		/* 217 = stav		*/		
	"lstatv",		/* 218 = lstav 		*/			
	"fstatv",		/* 219 = fstav 		*/			
	"getattrlist",		/* 220 = getattrlist 	*/		
	"setattrlist",		/* 221 = setattrlist 	*/		
	"getdirentryattr",	/* 222 = getdirentryattr*/	
	"exchangedata",		/* 223 = exchangedata   */			
	"checkuseraccess",	/* 224 - checkuseraccess*/
	"searchfs",		/* 225 = searchfs */
	"#226",			/* 226 = nosys */
	"#227",			/* 227 = nosys */
	"#228",			/* 228 = nosys */
	"#229",			/* 229 = nosys */
	"#230",			/* 230 = nosys */
		
	/*
	 * 216 - 230 are all reserved for suppoorting HFS File System
	 * Semantics.  225-230 are reserved for future use.
	 */
	"watchevent",		/* 231 = watchevent */
	"waitevent",		/* 232 = waitevent */
	"modwatch"			/* 233 = modwatch */
};
