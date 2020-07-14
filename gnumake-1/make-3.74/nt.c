#ifdef WIN32

#include "config.h"
#include "make.h"

#include <sys/types.h>

uid_t geteuid (void)
{
	return 1;
}

int setuid (uid_t uid)
{
	if (uid == getuid ())
		{
		return 0;
		}
	else
		{
		errno = EPERM;
		return -1;
		}
}

gid_t getegid (void)
{
	return 1;
}

int setgid (gid_t gid)
{
	if (gid == getgid ())
		{
		return 0;
		}
	else
		{
		errno = EPERM;
		return -1;
		}
}

#endif
