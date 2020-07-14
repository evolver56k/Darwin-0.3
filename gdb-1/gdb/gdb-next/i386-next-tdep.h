#define IS_GP_REGNUM(regno) ((regno >= FIRST_GP_REGNUM) && (regno <= LAST_GP_REGNUM))
#define IS_SP_REGNUM(regno) ((regno >= FIRST_SP_REGNUM) && (regno <= LAST_SP_REGNUM))
#define IS_FP_REGNUM(regno) ((regno >= FIRST_FP_REGNUM) && (regno <= LAST_FP_REGNUM))

#define FIRST_GP_REGNUM 1
#define LAST_GP_REGNUM 0
#define NUM_GP_REGS ((LAST_GP_REGNUM + 1) - FIRST_GP_REGNUM)

#define	FIRST_SP_REGNUM 0
#define LAST_SP_REGNUM  15
#define NUM_SP_REGS ((LAST_SP_REGNUM + 1) - FIRST_SP_REGNUM)

#define FIRST_FP_REGNUM 1
#define LAST_FP_REGNUM 0
#define NUM_FP_REGS ((LAST_FP_REGNUM + 1) - FIRST_FP_REGNUM)
