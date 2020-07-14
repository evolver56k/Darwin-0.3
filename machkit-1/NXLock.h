/*  Copyright (c) 1991, 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * NXLock.h. Standard lock object.
 *
 */

@protocol NXLock
- lock;     // acquire lock (enter critical section)
- unlock;   // release lock (leave critical section)
@end
 
/* Lock protocol
 *      send "lock" message upon entering critical section
 *      send "unlock" message to leave critical section
 *
 * There are four classes with different implementations and
 * performance characteristics.  There are example usages preceeding
 * each class definition.
 *
 * Use NXLock to protect regions of code that can consume long
 * periods of time (disk I/O, heavy computation.
 *
 * Use NXConditionLock for those cases where you wish only certain threads
 * to awaken based on some condition you define, or for those cases
 * when you have both short and long critical sections.
 *
 * Use NXSpinLock to protect short regions of critical code
 * (short in terms of time spent in the region). Useful primarily on
 * multiple processors.
 *
 * Use NXRecursiveLock to protect regions of code or data that may
 * be accessed by the same thread.  The lock will not block if it
 * is already held by the same thread.  This does not provide mutual
 * exclusion with naive signal handlers...
 *
 */
 
#import <objc/Object.h>

/* NXSpinLock
 * used to protect global objects held for short periods
 *  NXSpinLock  *glob = [NXSpinLock new];   // done once!
 *
 *  [glob lock];
 *  ... fuss with global data
 *  [glob unlock];
 */

@interface NXSpinLock : Object <NXLock> {
    void *      _priv;
}
@end


/*
 * Usage:
 * 
 * Producer:
 *
 *  id condLock = [NXConditionLock new];
 *  [condLock lock];
 *  ...manipulate global data...
 *  [condLock unlockWith:NEW_STATE];
 *
 * Consumer:
 *  
 *  ...The following sleeps until the producer does the unlockWith: such
 *     that DESIRED_STATE == NEW_STATE.
 *  [condLock lockWhen:DESIRED_STATE];
 *  ...manipulate global data if necessary...
 *  [condLock unlock];
 *
 *  ...OR...
 *
 *  [condLock lockWhen:DESIRED_STATE];
 *  ...manipulate global data if necessary, then notify other lock users
 *     of change of state.
 *  [condLock unlockWith:NEW_STATE];
 *
 * The value of 'condition' is user-dependent.
 *
 * All 4 combinations of {lock,lockUntil} and {unlock,unlockWith} are legal,
 * i.e.,
 *  {
 *      [condLock lock];
 *      ...
 *      [condLock unlock];
 *  }
 *  
 *  {
 *      [condLock lockWhen:SOME_CONDITION];
 *      ...
 *      [condLock unlock];
 *  }
 *
 *  {
 *      [condLock lock];
 *      ...
 *      [condLock unlockWith:SOME_CONDITION];
 *  }
 *
 *  {
 *      [condLock lockWhen:SOME_CONDITION];
 *      ...
 *      [condLock unlockWith:ANOTHER_CONDITION];
 *  }
 */
 
@interface NXConditionLock : Object <NXLock> {
    void    *_priv;
}

- initWith : (int)condition;        // init & set condition variable
- (int) condition;
- lockWhen : (int)condition;        // acquire lock when 
                    //   conditionVar == condition
- unlockWith : (int)condition;      // release lock and update conditionVar

@end



/* NXLock
 * usage for objects held for longer periods (I/O, heavy computation, ...)
 *
 *  NXLock  *glob = [NXLock new];       // done once!
 *
 *  [glob lock];
 *  ... long time of fussing with global data
 *  [glob unlock];
 */
 
@interface NXLock : Object <NXLock> {
    void    *_priv;
}

@end

/* NXRecursiveLock
 * used for locks that need to be reacquired by the same thread 
 *
 *  NXRecursiveLock  *glob = [NXRecursiveLock new];       // done once!
 *
 *  [glob lock];
 *  ... long time of fussing with global data
 *		[glob lock] from some interior routine
 *		[glob unlock] 
 *  [glob unlock];
 */
    
@interface NXRecursiveLock : Object <NXLock> {
    void    *_priv;
}

@end
