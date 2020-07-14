#include "nextstep-nat-dyld-process.h"

#include "nextstep-nat-dyld-info.h"
#include "nextstep-nat-dyld-path.h"
#include "nextstep-nat-inferior.h"
#include "nextstep-nat-mutils.h"

#include "defs.h"
#include "inferior.h"
#include "symfile.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "objfiles.h"

#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_debug.h>

#include <assert.h>
#include <string.h>

#include "mach-o.h"

extern bfd *exec_bfd;

void dyld_update_section_tables (struct dyld_objfile_info *result, struct target_ops *target)
{
  unsigned int exec_nsections = 0;
  unsigned int dylib_nsections = 0;
  struct section_table *exec_sections;
  unsigned int i;

  if (target->to_sections != NULL) {
    assert (target->to_sections_end >= target->to_sections);
    /* free (target->to_sections); */
    target->to_sections = NULL;
    target->to_sections_end = NULL;
  }

  if (exec_bfd == NULL) { 
    return;
  }

  if (build_section_table (exec_bfd, &target->to_sections, &target->to_sections_end)) {
    error ("\"%s\": can't find the file sections: %s", 
	   exec_bfd->filename, bfd_errmsg (bfd_get_error ()));
  }
  
  assert (target->to_sections != NULL); 
  assert (target->to_sections_end >= target->to_sections); 
  exec_nsections = target->to_sections_end - target->to_sections;

  if (result->sections == NULL) {
    return;
  }

  assert (result->sections_end >= result->sections);
  dylib_nsections = result->sections_end - result->sections;

  exec_sections = xmalloc ((exec_nsections + dylib_nsections) * sizeof (struct section_table));
  memcpy (exec_sections, target->to_sections, (exec_nsections * sizeof (struct section_table)));

  for (i = 0; i < dylib_nsections; i++) {
    struct section_table *s = exec_sections + exec_nsections + i;
    s->addr = result->sections[i].addr;
    s->endaddr = result->sections[i].endaddr;
    s->the_bfd_section = result->sections[i].the_bfd_section;
    assert (result->sections[i].objfile != NULL);
    s->bfd = result->sections[i].objfile->obfd;
  }

  free (target->to_sections);
  target->to_sections = NULL;

  target->to_sections = exec_sections;
  target->to_sections_end = exec_sections + exec_nsections + dylib_nsections;

  return;
}

/* relocate the section to match where the container is in memory. This seems
   to get out of whack when dealing with a shared library that is not pre-bound.
   For those libraries the sections are zero based while the container has been
   loaded at some address. This causes GDB to get confiused when doing a step or
   next in such a library (I'm not postive why, all I know is that this fixes it).
*/
static void
check_section_in_entry(struct obj_section* section, struct dyld_objfile_entry* entry)
{
    if (section->addr < entry->loaded_addr)
      {
        dyld_debug("section %s in %s at %#x, entry at %#x\n",
                   section->the_bfd_section->name,
                   entry->loaded_name,
                   (unsigned long) section->addr,
                   (unsigned long) entry->loaded_addr);
        section->addr += entry->loaded_addr;
        section->endaddr += entry->loaded_addr;
      }
}

void dyld_merge_section_tables (struct dyld_objfile_info *result)
{
  unsigned int nsections = 0;
  unsigned int cursection = 0;
  unsigned int i, j;

  if (result->sections != NULL) {
    assert (result->sections_end >= result->sections);
    free (result->sections);
    result->sections = NULL;
    result->sections_end = NULL;
  }
  
  for (i = 0; i < result->nents; i++) {
    
    struct dyld_objfile_entry *e = &result->entries[i];
    
    if (! e->allocated) { continue; }

    if (e->objfile == NULL) { continue; }
    if (e->objfile->sections == NULL) { continue; }

    assert (e->objfile->sections_end >= e->objfile->sections);
    
    nsections += e->objfile->sections_end - e->objfile->sections;
  }

  result->sections = (struct obj_section *) xmalloc (nsections * sizeof (struct obj_section));
  
  cursection = 0;
  for (i = 0; i < result->nents; i++) {

    struct dyld_objfile_entry *e = &result->entries[i];
    unsigned int cur_nsections;

    if (! e->allocated) { continue; }

    if (e->objfile == NULL) { continue; }
    if (e->objfile->sections == NULL) { continue; }

    assert (e->objfile->sections_end >= e->objfile->sections);

    cur_nsections = e->objfile->sections_end - e->objfile->sections;
    for (j = 0; j < cur_nsections; j++) {
      assert (e->objfile->sections[j].objfile != NULL);
      check_section_in_entry(&e->objfile->sections[j], e);
      result->sections[cursection + j] = e->objfile->sections[j];
    }
    cursection += cur_nsections;
  }
  assert (cursection == nsections);

  result->sections_end = result->sections + cursection;
}

