#include "defs.h"
#include "gdbcore.h"
#include "target.h"
#include "embedded-symbol.h"
#include "traceback.h"

static enum language traceback_table_languages[] =
{
  language_c,		/* TB_C */
  language_fortran,	/* TB_FORTRAN */
  language_unknown,	/* TB_PASCAL */
  language_unknown,	/* TB_ADA */
  language_unknown,	/* TB_PL1 */
  language_unknown,	/* TB_BASIC */
  language_unknown,	/* TB_LISP */
  language_unknown,	/* TB_COBOL */
  language_m2,		/* TB_MODULA2 */
  language_cplus,       /* TB_CPLUSPLUS */
  language_unknown,     /* TB_RPG */
  language_unknown,     /* TB_PL8 */
  language_asm          /* TB_ASM */
};

/* FIXME: this assumes the entire traceback table will fit within 1024
   characters of the address being examined */

/* FIXME: this assumes big-endian integers */

static struct embedded_symbol*
search_for_traceback_table (CORE_ADDR pc)
{
  static long buffer[1024];
  static embedded_symbol symbol;
  embedded_symbol *result = NULL;
  int status;

  symbol.name = NULL;
  symbol.language = language_unknown;
    
  status = target_read_memory (pc, (char*) buffer, sizeof (buffer));

  if (status == 0) {

    TracebackTblPtr traceback_table = NULL; 
    int index;
        
    for (index = 0; index < 1024; index++) {
      if (buffer[index] == 0) {
	/* bump past NULL code terminator */
	traceback_table = (TracebackTblPtr) &buffer[++index];
	break;
      }
    }

    if (traceback_table != NULL) {
      /* skip fixed portion (in words) */
      int skip = (sizeof (TracebackTbl) / sizeof(long));

      if (traceback_table->fixedparms) { skip++; }
      if (traceback_table->flags5 & floatparms) { skip++; }
      if (traceback_table->flags1 & has_tboff) { skip++; }
      if (traceback_table->flags2 & int_hndl) { skip++; }
      if (traceback_table->flags1 & has_ctl) {
	AutoAnchorsPtr autoAnchors = NULL;
	/* include ctl_info in count */
	if (((skip * 4) + sizeof (struct AutoAnchors)) > (sizeof (buffer))) { 
	  return NULL;
	}
	autoAnchors = (AutoAnchorsPtr) &buffer[index + skip];
	skip += (autoAnchors->ctl_info + 1); 					
      }
      if (traceback_table->flags2 & name_present) {
	static char functionName[64];
	RoutineNamePtr routineName = (RoutineNamePtr) &buffer[index + skip];
                
	memcpy (functionName, routineName->name, min (routineName->name_len, 63));
	functionName[min(routineName->name_len, 63)] = '\0';

	if (traceback_table->lang <= TB_ASM) {
	  symbol.language = traceback_table_languages[traceback_table->lang];
	} else {
	  symbol.language = language_unknown;
	}

	/* strip leading period inserted by compiler */
	if (functionName[0] == '.') {
	  symbol.name = &functionName[1];
	} else {
	  symbol.name = &functionName[0];
	}
	result = &symbol;
      }
    }
  }
  
  return result;
}

struct embedded_symbol 
*search_for_embedded_symbol (CORE_ADDR pc)
{
  /* Right now, this only knows about AIX and PowerMac style traceback
     tables so don't bother with complicated logic for registering
     other types of embedded symbol searching routines */

  return search_for_traceback_table(pc);
}

