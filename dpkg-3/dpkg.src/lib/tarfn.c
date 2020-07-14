/*
 * Functions for extracting tar archives.
 * Bruce Perens, April-May 1995
 * Copyright (C) 1995 Bruce Perens
 * This is free software under the GNU General Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "tarfn.h"
#include "libdpkg-int.h"

struct TarHeader {
  char Name[100];
  char Mode[8];
  char UserID[8];
  char GroupID[8];
  char Size[12];
  char ModificationTime[12];
  char Checksum[8];
  char LinkFlag;
  char LinkName[100];
  char MagicNumber[8];
  char UserName[32];
  char GroupName[32];
  char MajorDevice[8];
  char MinorDevice[8];
};
typedef struct TarHeader TarHeader;

static const unsigned int TarChecksumOffset
  = (unsigned int) &(((TarHeader *) 0)->Checksum);

/* Octal-ASCII-to-long */

static long OtoL (const char * s, int size)
{
  int n = 0;

  while (*s == ' ') {
    s++;
    size--;
  }

  while ((--size >= 0) && (*s >= '0') && (*s <= '7')) {
    n = (n * 010) + (*s++ - '0');
  }

  return n;
}

static int DecodeTarHeader (char *block, TarInfo *d)
{
  TarHeader *h = (TarHeader *) block;
  unsigned char *s = (unsigned char *)block;
  struct passwd *passwd = NULL;
  struct group *group = NULL;
  unsigned int i;
  long sum;
  long checksum;

  d->Name = h->Name;
  d->LinkName = h->LinkName;
  d->Mode = (mode_t) OtoL (h->Mode, sizeof (h->Mode));
  d->Size = (size_t) OtoL (h->Size, sizeof (h->Size));
  d->ModTime = (time_t) OtoL (h->ModificationTime, sizeof (h->ModificationTime));
  d->Device =
  ((OtoL (h->MajorDevice, sizeof(h->MajorDevice)) & 0xff) << 8)
  | (OtoL(h->MinorDevice, sizeof(h->MinorDevice)) & 0xff);
  checksum = OtoL (h->Checksum, sizeof (h->Checksum));
  d->UserID = (uid_t) OtoL (h->UserID, sizeof (h->UserID));
  d->GroupID = (gid_t) OtoL (h->GroupID, sizeof (h->GroupID));
  d->Type = (TarFileType) h->LinkFlag;
  
  if (*h->UserName) {
    passwd = getpwnam (h->UserName);
    if (passwd) {
      d->UserID = passwd->pw_uid;
    }
  }

  if (*h->GroupName) {
    group = getgrnam (h->GroupName);
    if (group) {
      d->GroupID = group->gr_gid;
    }
  }

  sum = ' ' * sizeof (h->Checksum); /* Treat checksum field as all blank */
  for (i = TarChecksumOffset; i > 0; i--) {
    sum += *s++;
  }
  s += sizeof (h->Checksum);	/* Skip the real checksum field */
  for (i = (512 - TarChecksumOffset - sizeof (h->Checksum)); i > 0; i-- ) {
    sum += *s++;
  }

  return (sum == checksum);
}

int TarExtractor (void *userData, const TarFunctions *functions)
{
  TarInfo h;
  char buffer[512];

  char *longName = NULL;
  char *longNameFree = NULL;
  int longNameSize = -1;
  int longNameOffset = -1;

  char *longLink = NULL;
  char *longLinkFree = NULL;
  int longLinkSize = -1;
  int longLinkOffset = -1;

  int nameLength = -1;
  int status = -1;
  int nread = -1;

  h.UserData = userData;

  for (;;) {

    nread = functions->Read (userData, buffer, 512);
    if (nread != 512) { break; }
    
    if (! DecodeTarHeader (buffer, &h)) {
      if (h.Name[0] == '\0') {
	return 0;		/* End of tape */
      } else {
	errno = 0;		/* Indicates broken tarfile */
	return -1;		/* Header checksum error */
      }
    }

    if (longName != NULL) { 
      h.Name = longName;
      longNameFree = longName;
      longName = NULL;
    }

    if (longLink != NULL) { 
      h.LinkName = longLink;
      longLinkFree = longLink;
      longLink = NULL;
    }

    nameLength = strlen (h.Name);

    switch (h.Type) {

    case GNULongFilename:
      longNameOffset = 0;
      longNameSize = (((h.Size + 1 + 511) / 512) * 512);
      longName = (char *) malloc (longNameSize + 1);
      if (longName == NULL) {
	;
      }
      while (longNameSize > 0) {
	nread = functions->Read (userData, longName + longNameOffset, 512);
	if (nread != 512) { break; }
	longNameSize -= 512;
	longNameOffset += 512;
      }
      status = 0;
      break;

    case GNULongLinkname:
      longLinkOffset = 0;
      longLinkSize = (((h.Size + 1 + 511) / 512) * 512);
      longLink = (char *) malloc (longLinkSize + 1);
      if (longLink == NULL) {
	;
      }
      while (longLinkSize > 0) {
	nread = functions->Read (userData, longLink + longLinkOffset, 512);
	if (nread != 512) { break; }
	longLinkSize -= 512;
	longLinkOffset += 512;
      }
      status = 0;
      break;

    case NormalFile0:
    case NormalFile1: 
      /* Compatibility with pre-ANSI ustar */
      if (h.Name[nameLength - 1] != '/') {
	status = (*functions->ExtractFile) (&h);
	break;
      }
      /* Else, Fall Through */
    case Directory:
      h.Name[nameLength - 1] = '\0';
      status = (*functions->MakeDirectory) (&h);
      break;
    case HardLink:
      status = (*functions->MakeHardLink) (&h);
      break;
    case SymbolicLink:
      status = (*functions->MakeSymbolicLink) (&h);
      break;
    case CharacterDevice:
    case BlockDevice:
    case FIFO:
      status = (*functions->MakeSpecialFile) (&h);
      break;
    default:
      errno = 0;		/* Indicates broken tarfile */
      return -1;		/* Bad header field */
    }

    if (longNameFree != NULL) { 
      free (longNameFree);
      longNameFree = NULL;
    }

    if (longLinkFree != NULL) { 
      free (longLinkFree);
      longLinkFree = NULL;
    }

    if (status != 0) {
      return status;		/* Pass on status from coroutine */
    }
  }

  if (status > 0) {		/* Read partial header record */
    errno = 0;			/* Indicates broken tarfile */
    return -1;
  } else {
    return status;		/* Whatever I/O function returned */
  }
}
