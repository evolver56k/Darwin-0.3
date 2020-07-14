
#pragma options align=mac68k
#include <CodeFragmentInfoPriv.h>
#pragma options align=reset
// this is to keep bfd from barfing
#define TRUE_FALSE_ALREADY_DEFINED

#include <assert.h>

#include "defs.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "frame.h"
#include "breakpoint.h"
#include "symtab.h"
#include "annotate.h"
#include "target.h"

#include "nextstep-nat-inferior.h"
#include "nextstep-nat-cfm.h"
#include "nextstep-nat-dyld-info.h"

#define IS_BC_x(instruction) \
((((instruction) & 0xFC000000) >> 26) == 16)

#define IS_B_x(instruction) \
((((instruction) & 0xFC000000) >> 26) == 18)

#define IS_BCLR_x(instruction) \
(((((instruction) & 0xFC000000) >> 26) == 19) && ((((instruction) & 0x000007FE) >> 1) == 16))

#define IS_BCCTR_x(instruction) \
(((((instruction) & 0xFC000000) >> 26) == 19) && ((((instruction) & 0x000007FE) >> 1) == 528))

#define WILL_LINK(instruction) \
(((instruction) & 0x00000001) != 0)

static CORE_ADDR
read_lr(void)
{
    CORE_ADDR result;

    fetch_inferior_registers(LR_REGNUM);
    result = extract_unsigned_integer (registers + (REGISTER_BYTE (LR_REGNUM)), sizeof (REGISTER_TYPE));

    return result;
}

static void
set_return_break(CORE_ADDR returnAddress)
{
    struct symtab_and_line	sal;
    struct breakpoint*		breakpoint;

    INIT_SAL(&sal);

    sal.pc = returnAddress;
    breakpoint = set_momentary_breakpoint(sal, NULL, bp_breakpoint);
    breakpoint->disposition = del;
}

static void
metrowerks_step_command(char* args, int from_tty)
{
    CORE_ADDR	boundsStart, boundsStop, currentPC;
    int			stepIn;

    boundsStart = strtoul(args, &args, 16);
    boundsStop = strtoul(args, &args, 16);
    stepIn = strtoul(args, &args, 16);

    /*	When single stepping in assembly, the plugin passes start+1 as the
        stop address. Round the stop address up to the next valid instruction
    */
    if ((boundsStop & ~0x3) != boundsStop)
    {
        boundsStop = ((boundsStop + 4) & ~0x3);
    }

    currentPC = read_pc();

    if (!((currentPC >= boundsStart) && (currentPC <= boundsStop)))
    {
        error("bad step boundaries\n");
    }

    while ((currentPC >= boundsStart) &&
           (currentPC < boundsStop))
    {
        unsigned long	currentInstruction;

        clear_proceed_status();

        target_read_memory(currentPC, (char*) &currentInstruction, sizeof(currentInstruction));
        if (!stepIn &&
            ((IS_BC_x(currentInstruction) ||
              IS_B_x(currentInstruction) ||
              IS_BCLR_x(currentInstruction) ||
              IS_BCCTR_x(currentInstruction)) &&
             WILL_LINK(currentInstruction)))
        {
            CORE_ADDR	returnAddress;
            step_1(FALSE, TRUE, NULL);

            returnAddress = read_lr();
            set_return_break(returnAddress);

            clear_proceed_status();
            proceed((CORE_ADDR) -1, TARGET_SIGNAL_DEFAULT, 0);
        }
        else
        {
            step_1(FALSE, TRUE, NULL);
        }
        
        currentPC = read_pc();
    }
}

/* This is the same (admittedly cheesey) logic used by the Mac OS 8.x
   MetroNub. It works most of the time though if you're only doing source
   level debuggging. To do this for real you would have to decode the
   instruction stream between the start of the fuction and the offset
   passed in here looking to see just exactly where the return address
   got put. That would be a pain in the ass.
*/
static void
metrowerks_step_out_command(char* args, int from_tty)
{
#define MFLR_R0 0x7C0802A6

    unsigned long	offset;
    CORE_ADDR		currentPC, returnAddress;
    unsigned long	firstInstruction;

    offset = strtoul(args, NULL , 16);

    currentPC = read_pc();
    target_read_memory(currentPC - offset, (char*) &firstInstruction, sizeof(firstInstruction));

    if ((0 == offset) || (MFLR_R0 != firstInstruction))
    {
        returnAddress = read_lr();
    }
    else
    {
        CORE_ADDR		stackPointer;
        unsigned long	savedStackPointer, savedReturnAddress;

        stackPointer = read_sp();
        target_read_memory(stackPointer, (char*) &savedStackPointer, sizeof(savedStackPointer));
        target_read_memory((CORE_ADDR) (savedStackPointer + 8), (char*) &savedReturnAddress, sizeof(savedReturnAddress));

        returnAddress = savedReturnAddress;
    }

    set_return_break(returnAddress);

    clear_proceed_status();
    proceed((CORE_ADDR) -1, TARGET_SIGNAL_DEFAULT, 0);

    currentPC = read_pc();
}

static void
metrowerks_break_command(char* args, int from_tty)
{
    char*			fragmentName;
    int				nameLength;
    unsigned long	section;
    unsigned long	offset;
    CFMContainer*	cfmContainer = NULL;
    CFMSection*   	cfmSection = NULL;
    struct symtab_and_line sal = {0, 0};
    struct breakpoint *b = set_raw_breakpoint(sal);

    fragmentName = args;
    while (*args != ' ') args++;
    nameLength = (args - fragmentName);
    
    section = strtoul(args, &args, 16);
    offset = strtoul(args, &args, 16);

    cfmContainer = CFM_FindContainerByName(fragmentName, nameLength);
    if (NULL != cfmContainer)
    {
        cfmSection = CFM_FindSection(cfmContainer, kCFContNormalCode);
    }

    set_breakpoint_count(get_breakpoint_count() + 1);
    b->number = get_breakpoint_count();
    b->inserted = 0;
    b->disposition = donttouch;
    b->type = bp_breakpoint;
    b->addr_string = savestring(fragmentName, strlen(fragmentName));

    if (NULL != cfmSection)
    {
        b->address = ((unsigned long) cfmSection->mSection.address + offset);
        b->enable = enabled;
    }
    else
    {
        b->address = -1;
        b->enable = shlib_disabled;
    }

    annotate_breakpoint(b->number);

    if (modify_breakpoint_hook)
    {
        modify_breakpoint_hook(b);
    }
}

bfd*
FindContainingBFD(unsigned long address)
{
    struct target_stack_item*	aTarget;
    bfd*						result = NULL;

    for (aTarget = target_stack;
         (result == NULL) && (aTarget != NULL);
         aTarget = aTarget->next)
    {
        struct section_table* sectionTable;

        if ((NULL == aTarget->target_ops) ||
            (NULL == aTarget->target_ops->to_sections))
        {
            continue;
        }
        
        for (sectionTable = &aTarget->target_ops->to_sections[0];
             (result == NULL) && (sectionTable < aTarget->target_ops->to_sections_end);
             sectionTable++)
        {
            if ((address >= sectionTable->addr) &&
                (address < sectionTable->endaddr))
            {
                result = sectionTable->bfd;
            }
        }
    }

    return result;
}

static void
metrowerks_address_to_name_command(char* args, int from_tty)
{
    unsigned long  	address;
    CFMSection*	 	cfmSection;
    bfd*			aBFD;

    address = strtoul(args, NULL, 16);

    if (annotation_level > 1)
    {
        printf("\n\032\032fragment-name ");
    }

    cfmSection = CFM_FindContainingSection((char*) address);
    if (NULL != cfmSection)
    {
        printf_unfiltered("%.*s\n",
                          cfmSection->mContainer->mContainer.name[0],
                          &cfmSection->mContainer->mContainer.name[1]);
        return;
    }

    aBFD = FindContainingBFD(address);
    if (NULL != aBFD)
    {
        printf_unfiltered("%s\n", aBFD->filename);
        return;
    }

    printf_unfiltered("[unknown]\n");
}

void
_initialize_metrowerks(void)
{
    add_com("metrowerks-step", class_obscure, metrowerks_step_command, "GDB as MetroNub command");
    add_com("metrowerks-step-out", class_obscure, metrowerks_step_out_command, "GDB as MetroNub command");
    add_com("metrowerks-break", class_breakpoint, metrowerks_break_command, "GDB as MetroNub command");
    add_com("metrowerks-address-to-name", class_obscure, metrowerks_address_to_name_command, "GDB as MetroNub command");
}
