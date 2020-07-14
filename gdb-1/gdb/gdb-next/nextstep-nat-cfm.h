#ifndef _NEXTSTEP_NAT_CFM_H_
#define _NEXTSTEP_NAT_CFM_H_

#pragma options align=mac68k
#include <CodeFragmentInfoPriv.h>
#pragma options align=reset

extern void cfm_info_init (next_cfm_info* cfm_info);
extern void next_init_cfm_info_api (struct next_inferior_status* status);
extern void next_handle_cfm_event (struct next_inferior_status* status, void* message);

typedef struct CFMSection CFMSection;
typedef struct CFMContainer CFMContainer;
typedef struct CFMConnection CFMConnection;
typedef struct CFMClosure CFMClosure;

struct CFMSection
{
  CFMSection *mNext;
  CFragSectionInfo mSection;
  CFMContainer *mContainer;
};

struct CFMContainer
{
  CFMContainer *mNext;
  CFMSection *mSections;
  CFragContainerInfo mContainer;
};

struct CFMConnection
{
  CFMConnection *mNext;
  CFMContainer *mContainer;
  CFragConnectionInfo mConnection;
};

struct CFMClosure
{
  CFMClosure *mNext;
  CFMConnection *mConnections;
  CFragClosureInfo mClosure;
};

CFMContainer *CFM_FindContainerByName (char *name, int length);
CFMSection *CFM_FindSection (CFMContainer *container, CFContMemoryAccess accessType);
CFMSection *CFM_FindContainingSection (char *address);

#endif /* _NEXTSTEP_NAT_CFM_H_ */
