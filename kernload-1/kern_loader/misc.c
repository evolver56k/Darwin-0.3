/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 *  4-Sep-90  Gregg Kellogg (gk) at NeXT
 *	Added kern_serv_error and kern_serv_error_string functions.
 *
 * 27-Oct-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#import "server.h"
#import "misc.h"
#import "log.h"
#import <stddef.h>
#import <stdlib.h>
#import <stdio.h>
#import <strings.h>
#import <objc/hashtable.h>
#import <mach/mach_error.h>
#import <kernserv/kern_server_types.h>

extern const struct section *
getsectbynamefromheader (
	struct mach_header	*mhp,
	const char		*segname,
	const char		*sectname);

const void *getSectDataFromHeader (
	const server_t	*server,
	const char	*segname,
	const char	*sectname,
	int		*size)
{
	const struct section *sect;
	const char *addr;
	sect = getsectbynamefromheader(server->mheader, segname, sectname);
	if (!sect || !sect->size)
		return NULL;

	*size = sect->size;
	addr = (const char *)server->mheader + sect->offset;

	return (void *)addr;
}

const char *getMachoString (
	const server_t	*server,
	const char	*segname,
	const char	*sectname)
{
	const struct section *sect;
	const char *s;

	sect = getsectbynamefromheader(server->mheader, segname, sectname);
	if (!sect || !sect->size)
		return NULL;

	s = (const char *)server->mheader + sect->offset;

	return NXUniqueStringWithLength(s, sect->size);
}

const char *getMachoData (
	const server_t	*server,
	const char	*segname,
	const char	*sectname)
{
	const struct section *sect;
	char *data;
	const char *temp;

	sect = getsectbynamefromheader(server->mheader, segname, sectname);
	if (!sect || !sect->size)
		return NULL;

	temp = (const char *)server->mheader + sect->offset;

	data = malloc(sect->size+1);
	memcpy(data, temp, sect->size);
	data[sect->size] = '\0';

	return (const char *)data;
}

mutex_t taskNameLock;
NXHashTable *taskNameHashTable;
struct TaskName {
	NXAtom	name;
	task_t	task;
};

/*
 * Given a name look up the task_port associated with it (if any).
 */
kern_return_t task_by_name(const char *name, task_t *result)
{
	struct TaskName *taskName;

	mutex_lock(taskNameLock);
	taskName = NXHashGet(taskNameHashTable, &name);
	mutex_unlock(taskNameLock);
	if (!taskName) {
		*result = PORT_NULL;
		return KERN_FAILURE;
	}

	*result = taskName->task;
	return KERN_SUCCESS;
}

/*
 * Save this task/name combination.
 */
boolean_t save_task_name(const char *name, task_t task)
{
	struct TaskName *taskName = malloc(sizeof(*taskName));
	struct TaskName *oldTaskName = NULL;

	mutex_lock(taskNameLock);

	taskName->name = NXUniqueString(name);
	taskName->task = task;
	oldTaskName = NXHashInsertIfAbsent(taskNameHashTable, taskName);
	if (oldTaskName != taskName) {
		free(taskName);
		mutex_unlock(taskNameLock);
		return FALSE;
	}
	mutex_unlock(taskNameLock);
	return TRUE;
}

/*
 * If the task_port goes away, forget this association.
 */
boolean_t delete_task_name(task_t task)
{
	struct TaskName *taskName;
	NXHashState state;

	mutex_lock(taskNameLock);
	state = NXInitHashState(taskNameHashTable);
	while (NXNextHashState(taskNameHashTable, &state, (void **)&taskName))
		if (taskName->task == task) {
			/*
			 * Unhash this element and free it.
			 */
			free(NXHashRemove(taskNameHashTable, taskName));
			mutex_unlock(taskNameLock);
			return TRUE;
		}

	mutex_unlock(taskNameLock);
	return FALSE;
}

mutex_t symfileLock;
int symfileLock_taken;
condition_t symfileLock_wait;


#define symfile_lock()  mutex_lock(symfileLock);                           \
                        while (symfileLock_taken){                           \
                                condition_wait(symfileLock_wait, symfileLock);\
                        }                                                  \
                        symfileLock_taken = 1;                             \
                        mutex_unlock(symfileLock);

#define symfile_unlock() mutex_lock(symfileLock);                         \
                        symfileLock_taken = 0;                            \
                        condition_signal(symfileLock_wait);               \
                        mutex_unlock(symfileLock);



NXHashTable *symfileHashTable;
struct FileSymHash {
	NXAtom		filename;
	NXHashTable	*symhash;
};

struct SymValueHash {
	NXAtom		symname;
	vm_address_t	symvalue;
};

/*
 * Given a filename and symbolname lookup the value of the symbol.
 */
kern_return_t sym_value (
	const char	*filename,
	const char	*symname,
	vm_address_t	*result)
{
	struct FileSymHash *fileSymHash;
	struct SymValueHash *symValueHash;

	symfile_lock();
	fileSymHash = NXHashGet(symfileHashTable, &filename);
	if (!fileSymHash) {
		symfile_unlock();
		*result = 0;
		return KERN_FAILURE;
	}

	symValueHash = NXHashGet(fileSymHash->symhash, &symname);
	symfile_unlock();
	if (!symValueHash) {
		*result = 0;
		return KERN_FAILURE;
	}

	*result = symValueHash->symvalue;
	return KERN_SUCCESS;
}

/*
 * Save this task/name combination.
 */
boolean_t save_symvalue (
	const char	*filename,
	const char	*symname,
	vm_address_t	symvalue)
{
	struct FileSymHash *fileSymHash;
	struct SymValueHash *symValueHash;
	struct SymValueHash *oldSymValueHash = NULL;

	symfile_lock();

	fileSymHash = NXHashGet(symfileHashTable, &filename);
	if (!fileSymHash) {
		fileSymHash = malloc(sizeof(*fileSymHash));
		fileSymHash->filename = NXUniqueString(filename);
		fileSymHash->symhash =
			NXCreateHashTable(NXStrStructKeyPrototype, 0, NULL);
		NXHashInsert(symfileHashTable, fileSymHash);
	}

	symValueHash = malloc(sizeof(*symValueHash));
	symValueHash->symname = NXUniqueString(symname);
	symValueHash->symvalue = symvalue;

	oldSymValueHash = NXHashInsertIfAbsent(fileSymHash->symhash,
		symValueHash);

	symfile_unlock();
	if (oldSymValueHash != symValueHash) {
		free(symValueHash);
		return FALSE;
	}

	return TRUE;
}

/*
 * Obsolete symbols in the given file.
 */
boolean_t delete_symfile(const char *filename)
{
	struct FileSymHash *fileSymHash;
	struct SymValueHash *symValueHash;
	NXHashState state;

	symfile_lock();
	fileSymHash = NXHashGet(symfileHashTable, &filename);
	if (!fileSymHash) {
		symfile_unlock();
		return FALSE;
	}

	state = NXInitHashState(fileSymHash->symhash);

	while (NXNextHashState(fileSymHash->symhash, &state,
				(void **)&symValueHash))
	{
		/*
		 * Unhash this element and free it.
		 */
		free(NXHashRemove(fileSymHash->symhash, symValueHash));
	}

	symfile_unlock();
	return TRUE;
}

void hash_init(void)
{
    taskNameLock = mutex_alloc();
    mutex_init(taskNameLock);
    taskNameHashTable = NXCreateHashTable(NXStrStructKeyPrototype, 0, NULL);

    symfileLock = mutex_alloc();
    mutex_init(symfileLock);
    symfileLock_wait = condition_alloc();
    condition_init(symfileLock_wait);
    symfileHashTable = NXCreateHashTable(NXStrStructKeyPrototype, 0, NULL);
}

static const char *kern_serv_error_list[] = {
	"mal-formed message",
	"server not logging data",
	"port not recognized",
	"server version not supported",
};

const char *kern_serv_error_string(kern_return_t r)
{
	if (r < KERN_SERVER_ERROR)
		return mach_error_string(r);
	else
		return kern_serv_error_list[r - KERN_LOADER_NO_PERMISSION];
}

void kern_serv_error(const char *s, kern_return_t r)
{
	if (r < KERN_LOADER_NO_PERMISSION || r > KERN_LOADER_SERVER_WONT_LOAD)
		mach_error(s, r);
	else
		fprintf(stderr, "%s : %s (%d)\n", s,
			kern_serv_error_string(r), r);
}
