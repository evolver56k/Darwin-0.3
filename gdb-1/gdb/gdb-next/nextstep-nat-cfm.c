/* this is to keep bfd from barfing */
#define TRUE_FALSE_ALREADY_DEFINED

#include <assert.h>

#include "defs.h"
#include "breakpoint.h"

#include "nextstep-nat-inferior.h"
#include "nextstep-nat-inferior-util.h"
#include "nextstep-nat-mutils.h"
#include "nextstep-nat-cfm.h"

static void enable_breakpoints_in_containers(void);

static CFMClosure *gClosures = NULL;

extern OSStatus CCFM_SetInfoSPITarget (MPProcessID targetProcess, void *targetCFMHook, mach_port_t notifyPort);

static void
annotate_closure_begin (CFragClosureInfo *closure)
{
  if (annotation_level > 1)
    {
      printf_filtered ("\n\032\032closure-begin |");
      gdb_flush (gdb_stdout);
      fwrite (closure, sizeof (*closure), 1, stdout);
      printf_filtered ("|\n");
    }
  printf_filtered ("Got closure message from CFM with %d connections\n", closure->connectionCount);
  gdb_flush (gdb_stdout);
}

static void
annotate_closure_end (void)
{
  if (annotation_level > 1)
    {
      printf_filtered ("\n\032\032closure-end\n");
    }
  gdb_flush (gdb_stdout);
}

static void
annotate_connection (CFragConnectionInfo *connection)
{
  if (annotation_level > 1)
    {
      printf_filtered ("\n\032\032connection |");
      gdb_flush (gdb_stdout);
      fwrite (connection, sizeof (*connection), 1, stdout);
      printf_filtered ("|\n");
    }
  gdb_flush (gdb_stdout);
}

static void
annotate_container (CFragContainerInfo *container)
{
  if (annotation_level > 1)
    {
      printf_filtered ("\n\032\032container |");
      gdb_flush (gdb_stdout);
      fwrite (container, sizeof (*container), 1, stdout);
      printf_filtered ("|\n");
    }
  printf_filtered 
    ("CFrag container \"%.*s\" @ %#x for %#x with %d sections\n",
     container->name[0], &container->name[1],
     container->address, container->length, container->sectionCount);
  gdb_flush (gdb_stdout);
}

static void
annotate_section (ItemCount sectionIndex, CFragSectionInfo *section)
{
  if (annotation_level > 1)
    {
      printf_filtered ("\n\032\032section %d |", sectionIndex);
      gdb_flush (gdb_stdout);
      fwrite (section, sizeof (*section), 1, stdout);
      printf_filtered ("|\n");
    }
  printf_filtered
    ("    Section %d @ %#x for %#x\n",
     sectionIndex, section->address, section->length);
  gdb_flush (gdb_stdout);
}

void
cfm_info_init (next_cfm_info *cfm_info)
{
  cfm_info->info_api_cookie = NULL;
  cfm_info->cfm_send_right = MACH_PORT_NULL;
  cfm_info->cfm_receive_right = MACH_PORT_NULL;

  gClosures = NULL;
}

void
next_init_cfm_info_api (struct next_inferior_status *status)
{
  kern_return_t	result;

  if ((MACH_PORT_NULL == status->cfm_info.cfm_receive_right) &&
      (NULL != status->cfm_info.info_api_cookie) &&
      (MACH_PORT_NULL != status->task))
    {
      result = mach_port_allocate(mach_task_self(),
				  MACH_PORT_RIGHT_RECEIVE,
				  &status->cfm_info.cfm_receive_right);
      MACH_CHECK_ERROR(result);
	
      result = mach_port_allocate(status->task,
				  MACH_PORT_RIGHT_RECEIVE,
				  &status->cfm_info.cfm_send_right);
      MACH_CHECK_ERROR(result);
	

      result = mach_port_destroy (status->task, status->cfm_info.cfm_send_right);
      MACH_CHECK_ERROR(result);

      result = mach_port_insert_right(status->task,
				      status->cfm_info.cfm_send_right,
				      status->cfm_info.cfm_receive_right,
				      MACH_MSG_TYPE_MAKE_SEND);
      MACH_CHECK_ERROR(result);

      result = CCFM_SetInfoSPITarget((MPProcessID) status->task,
				     status->cfm_info.info_api_cookie,
				     status->cfm_info.cfm_send_right);
      MACH_CHECK_ERROR(result);
    }
}

void
next_handle_cfm_event (struct next_inferior_status* status, void* messageData)
{
#if !defined(__MACH30__)    
    msg_header_t		reply;
    msg_header_t*		message = (msg_header_t*) messageData;
#else
    mach_msg_header_t	reply;
    mach_msg_header_t*	message = (mach_msg_header_t*) messageData;
#endif    
    CFragNotifyInfo*	messageBody = (CFragNotifyInfo*) (message + 1);
    kern_return_t		result;
    ItemCount			numberOfConnections, totalCount, index;
    CFragConnectionID*	connections;
    CFragClosureInfo	closureInfo;
    CFMClosure*			closure;
    
    gdb_flush (gdb_stdout);	/* clean everything out before out annotations are printed */
    
    assert ((kCFragLoadClosureNotify == messageBody->notifyKind) ||
            (kCFragUnloadClosureNotify == messageBody->notifyKind));

    result = CFragGetClosureInfo(messageBody->u.closureInfo.closureID,
                                 kCFragClosureInfoVersion,
                                 &closureInfo);
    assert (noErr == result);
    annotate_closure_begin(&closureInfo);

    closure = xcalloc(1, sizeof(CFMClosure));
    assert (NULL != closure);
    closure->mClosure = closureInfo;

    result = CFragGetConnectionsInClosure(closureInfo.closureID,
                                          0,
                                          &numberOfConnections,
                                          NULL);
    assert (noErr == result);

    connections = xmalloc(numberOfConnections * sizeof(CFragConnectionID));
    assert (NULL != connections);

    result = CFragGetConnectionsInClosure(closureInfo.closureID,
                                          numberOfConnections,
                                          &totalCount,
                                          connections);
    assert (noErr == result);
    assert (totalCount == numberOfConnections);

    for (index = 0; index < numberOfConnections; ++index)
    {
        CFragConnectionInfo	connectionInfo;
        CFragContainerInfo	containerInfo;
        ItemCount			sectionIndex;
        CFMConnection*		connection;
        CFMContainer*		container;
        
        result = CFragGetConnectionInfo(connections[index],
                                        kCFragConnectionInfoVersion,
                                        &connectionInfo);
        assert (noErr == result);
        annotate_connection(&connectionInfo);

        connection = xcalloc(1, sizeof(CFMConnection));
        assert (NULL != connection);
        connection->mConnection = connectionInfo;
        connection->mNext = closure->mConnections;
        closure->mConnections = connection;

        result = CFragGetContainerInfo(connectionInfo.containerID,
                                       kCFragContainerInfoVersion,
                                       &containerInfo);
        assert (noErr == result);
        annotate_container(&containerInfo);

        container = xcalloc(1, sizeof(CFMContainer));
        assert (NULL != container);
        container->mContainer = containerInfo;
        connection->mContainer = container;

        for (sectionIndex = 0; sectionIndex < containerInfo.sectionCount; ++sectionIndex)
        {
            CFragSectionInfo	sectionInfo;
            CFMSection*			section;
            
            result = CFragGetSectionInfo(connections[index],
                                         sectionIndex,
                                         kCFragSectionInfoVersion,
                                         &sectionInfo);
            assert (noErr == result);
            annotate_section(sectionIndex, &sectionInfo);

            section = xcalloc(1, sizeof(CFMSection));
            assert (NULL != section);
            section->mSection = sectionInfo;
            section->mContainer = container;
            section->mNext = container->mSections;
            container->mSections = section;
        }
    }

    xfree (connections);

    annotate_closure_end();

    closure->mNext = gClosures;
    gClosures = closure;
       
    next_inferior_suspend_mach(status);
    enable_breakpoints_in_containers();

    bzero(&reply, sizeof(reply));
#if !defined (__MACH30__)    
    reply.msg_simple = TRUE;
    reply.msg_size = sizeof(reply);
    reply.msg_remote_port = message->msg_remote_port;
    reply.msg_local_port = MACH_PORT_NULL;

    result = msg_send(&reply, MSG_OPTION_NONE, MACH_MSG_TIMEOUT_NONE);
#else
    reply.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    reply.msgh_remote_port = message->msgh_remote_port;
    reply.msgh_local_port = MACH_PORT_NULL;

    result = mach_msg(&reply,
                      (MACH_SEND_MSG | MACH_MSG_OPTION_NONE),
                      sizeof(reply),
                      0,
                      MACH_PORT_NULL,
                      MACH_MSG_TIMEOUT_NONE,
                      MACH_PORT_NULL);
#endif    
    MACH_CHECK_ERROR(result);
}

void enable_breakpoints_in_containers (void)
{
    struct breakpoint *b;
    extern struct breakpoint* breakpoint_chain;

    for (b = breakpoint_chain; b; b = b->next)
    {
        if ((b->enable == shlib_disabled) && (b->address = -1))
        {
            char*			fragmentName;
            int				nameLength;
            char*			addr_string;
            unsigned long	section;
            unsigned long	offset;
            CFMContainer*	cfmContainer;

            addr_string = b->addr_string;
            fragmentName = addr_string;
            while (*addr_string != ' ') addr_string++;
            nameLength = (addr_string - fragmentName);

            section = strtoul(addr_string, &addr_string, 16);
            offset = strtoul(addr_string, &addr_string, 16);

            cfmContainer = CFM_FindContainerByName(fragmentName, nameLength);
            if (NULL != cfmContainer)
            {
                CFMSection*	cfmSection = CFM_FindSection(cfmContainer, kCFContNormalCode);
                if (NULL != cfmSection)
                {
                    b->address = (cfmSection->mSection.address + offset);
                    b->enable = enabled;
                }
            }
        }
    }
}

CFMContainer *CFM_FindContainerByName (char *name, int length)
{
    CFMContainer*	found = NULL;
    CFMClosure* 	closure = gClosures;
    
    while ((NULL == found) && (NULL != closure))
    {
        CFMConnection* connection = closure->mConnections;
        while ((NULL == found) && (NULL != connection))
        {
            if (NULL != connection->mContainer)
            {
                CFMContainer* container = connection->mContainer;
                if (0 == memcmp(name,
                                &container->mContainer.name[1],
                                min(container->mContainer.name[0], length)))
                {
                    found = container;
                    break;
                }
            }

            connection = connection->mNext;
        }

        closure = closure->mNext;
    }

    return found;
}

CFMSection *CFM_FindSection (CFMContainer *container, CFContMemoryAccess accessType)
{
  int done = false;
  CFMSection *section = container->mSections;

  while (!done && (NULL != section))
    {
      CFContMemoryAccess access = section->mSection.userAccess;
      if ((access & accessType) == accessType)
        {
	  done = true;
	  break;
        }
        
      section = section->mNext;
    }

  return (done ? section : NULL);
}

CFMSection *CFM_FindContainingSection (char *address)
{
  CFMSection*	section = NULL;
  CFMClosure*	closure = gClosures;

  while ((NULL == section) && (NULL != closure))
    {
      CFMConnection* connection = closure->mConnections;
      while ((NULL == section) && (NULL != connection))
        {
	  CFMContainer*	container = connection->mContainer;
	  CFMSection*		testSection = container->mSections;
	  while ((NULL == section) && (NULL != testSection))
            {
	      if ((address >= testSection->mSection.address) &&
		  (address < (testSection->mSection.address + testSection->mSection.length)))
                {
		  section = testSection;
		  break;
                }
                
	      testSection = testSection->mNext;
            }
            
	  connection = connection->mNext;
        }

      closure = closure->mNext;
    }

  return section;
}
