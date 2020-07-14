/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright 1993 NeXT Computer, Inc.
 * All rights reserved.
 */

#import "libsaio.h"
#import "memory.h"
#import "kernBootStruct.h"
#import "load.h"
#import "sarld.h"
#import "language.h"
#import "drivers.h"
#import "stringConstants.h"
#import "pci.h"

#import <ufs/fsdir.h>
#import <mach-o/fat.h>

static void sort_names(struct driver_info *, int);
extern char *LoadableFamilies;
static struct driver_info *loaded_drivers;
static int num_loaded;
extern BOOL errors;

static inline int isspace(char c)
{
	return (c == ' ' || c == '\t');
}

/*
 * A flag to let us know that a driver specified in
 * the system configuration was not found.
 */
int driverMissing;
static struct _missingDriver {
	char *bundleName;
	char *longName;
	char *version;
	char *tableName;
} missingDrivers[MAX_MISSING_DRIVERS];

static char *loadChoice1[] = {
"1 To Continue",
"2 To Load"
};

static char *loadChoice2[] = {
"1 To Load Driver",
"2 To Continue"
};

/*
 * Returns 0 if user picked a choice,
 * -1 if user wants another disk,
 * -2 if user wants to quit.
 */

int pickDrivers(
	struct driver_info *drivers,
	int ndrivers,
	int autoLoad,
	int instructionNum
)
{
	char *name, *version, *table;
	int i, fd, ret, number;
	int retcode = 0;
	char messageName[32], instructionName[32],
		 alternativeName[32], quitName[32];
	struct driver_load_data dl;

	sprintf(messageName, "DRIVER_MESSAGE_%d", instructionNum);
	sprintf(instructionName, "DRIVER_INSTRUCTION_%d", instructionNum);
	sprintf(alternativeName, "DRIVER_ALTERNATE_%d", instructionNum);
	sprintf(quitName, "DRIVER_QUIT_%d", instructionNum);

	if (autoLoad) {
		for (number=0; number < ndrivers; number++) {
			name = drivers[number].bundle;
			verbose("Loading %s device driver.\n",name);
			dl.name = name;
			if ((openDriverReloc(&dl)) >= 0) {
				ret = loadDriver(&dl);
				ret += linkDriver(&dl);
			} else
				ret = 0;
			if (ret >= 0) {
				table = drivers[number].configTable;
				addConfig(table);
				driverWasLoaded(name, table, NULL);
			}
		}
		goto out;
	}
	number = chooseDriverFromList(
		"",
		messageName,
		drivers, ndrivers,
		instructionName,
"Type %d to view a list of additional device drivers on this disk.\n\n",
		alternativeName,
		quitName);
	if (number == -1) {
		retcode = -1;
		goto out;
	} else if (number == -2) {
		return -2;
	}
	name = drivers[--number].bundle;
	version = drivers[number].version;
	for (i=0; i<num_loaded; i++) {
		if ((strcmp(name, loaded_drivers[i].bundle) == 0) &&
			(strcmp(version, loaded_drivers[i].version) == 0)) {
			verbose("That driver has already been loaded.\n");
			goto out;
		}
	}
	localPrintf("Loading %s device driver.\n",name);
	dl.name = name;
	if ((openDriverReloc(&dl)) >= 0) {
		ret = loadDriver(&dl);
		ret += linkDriver(&dl);
	} else {
		ret = 0;
	}
	if (ret < 0) {
		error("An error occurred while loading the %s device driver.\n",name);
		localPrintf("Press Return to continue.\n");
		printf("\n---> ");
		flushdev();
		while(getc() != '\r');
		printf("\n");
	} else {
		table = drivers[number].configTable;
		addConfig(table);
		printf(" \n");
		driverWasLoaded(name, table, NULL);
		verbose("The driver was loaded successfully.\n");
	}
out:
	return retcode;
}


/*
 * If any drivers are missing from the startup disk,
 * ask the user whether s/he wants to load drivers from a floppy.
 * Returns 1 if user wants to load missing drivers,
 * 0 otherwise.
 */
static int
askAboutMissingDrivers(void)
{
	int answer = -1;
	int i;
	
	if (driverMissing == 0)
		return 0;
		
	/* Now we've taken care of this error */
	errors = 0;
	while (answer == -1) {
		clearScreen();
		/* Localized text */
		localPrintf("Missing Drivers:");
		printf("\n");

		for (i=0; i<driverMissing; i++) {
			printf("  %s\n",missingDrivers[i].longName);
		}

//		printf("\n");
//		localPrintf(
//"Insert the floppy disk that contains these device drivers.\n");
	
		printf("\n");
		answer = chooseSimple(loadChoice2, 2, 1, 2);
	}
	return answer == 1;
}

/*
 * Returns 1 if user wants to load more drivers,
 * 0 otherwise.
 */
static int
askWhetherToLoadMoreDrivers(void)
{
	int answer = -1;

	while (answer == -1) {
		clearScreen();
//		  message("Load OPENSTEP Drivers",0);
//		  printf("\n");
		/* The text for the following message is in Localizable.strings. */
		localPrintf("Load Other Drivers?");
				
		printf("\n");
		answer = chooseSimple(loadChoice1, 2, 1, 2);
	}

	return (answer == 2);
}

static void
_set_dinfo(
	struct driver_info *dinfo,
	char *bundleName,
	char *configTable,
	char *tableName,
	char *locationTag
)
{
	char *hintTable, *path;
	
	dinfo->name = (char *)bundleLongName(bundleName,
								tableName);
	dinfo->bundle = newString(bundleName);
	if ((dinfo->version =
		newStringForStringTableKey(configTable, "Version")) == NULL)
			dinfo->version = newString("1.0");
	if (tableName) {
		/* Append location of default table -- */
		/* There are extra bytes in the table for this. */
#define TABLE_KEY		"\n\"Default Table\" = \"%s\";\n"	
		sprintf(configTable + strlen(configTable), TABLE_KEY, tableName);
	}
	if (dinfo->name) {
		/* Append long name of driver -- */
		/* There are extra bytes in the table for this. */
#define NAME_KEY		"\n\"Long Name\" = \"%s\";\n"
		sprintf(configTable + strlen(configTable), NAME_KEY, dinfo->name);
	}
	if (dinfo->locationTag) {
		/* Append location tag */
#define LOCATION_KEY	"\n\"Location\" = \"%s\";\n"
		sprintf(configTable + strlen(configTable), NAME_KEY,
			dinfo->locationTag);
	}
	path = malloc(256);
	sprintf(path, "%s/System.config/" INSTALL_HINTS
					"/%s.table", usrDevices(), bundleName);
	if (loadConfigFile(path, &hintTable, YES) == 0) {
		dinfo->configTable = hintTable;
		free(configTable);
	} else {
		dinfo->configTable = configTable;
	}
	free(path);
	dinfo->tableName = newString(tableName);
	dinfo->locationTag = newString(locationTag);
}

static BOOL
testIDs(const char *autoDetectIDs, _pci_slot_info_t *slot)
{
	char *optr = NULL, *ptr = (char *)autoDetectIDs;
	unsigned long int pid, ptest, pmask; // primary foundID, testID & mask
	unsigned long int sid, stest, smask; // secondary foundID, testID & mask

	ptest = pmask = stest = smask = sid = 0x00000000;
	pid = slot->pid;
	sid = slot->sid;
	if ( ((pid&0xffff) == 0xffff) || ((pid&0xffff) == 0x0000) )
		return NO;
	while (*ptr) {
		if (optr == ptr) return NO; // avoid an endless loop
		optr = ptr;
		if ((*ptr == ' ') || (*ptr == '\t')) {
			ptr++;
			ptest = pmask = 0x00000000;
			continue;
		}
		if (*ptr != ':') {
			ptest = strtoul(ptr, &ptr, 0);
			if (*ptr == '&') {
				ptr++;
				pmask = strtoul(ptr, &ptr, 0);
			} else
				pmask = 0xffffffff;
		}
		if (*ptr == ':') {
			ptr++;
			stest = strtoul(ptr, &ptr, 0);
			if (*ptr == '&') {
				ptr++;
				smask = strtoul(ptr, &ptr, 0);
			} else
				smask = 0xffffffff;
		} else
			stest = smask = 0x00;

		if ( ((pid&pmask)==(ptest&pmask)) && ((sid&smask)==(stest&smask)) )
			return YES;
	}
		
	return NO;
}


#define BUS_TYPE				"Bus Type"
#define AUTO_DETECT_IDS 		"Auto Detect IDs"
#define PCI_BUS 				"PCI"
#define PCI_LOCATION_STRING 	"Dev:%d Func:%d Bus:%d"

/*
 * This function sets a driver_info structure element for the
 * driver we've found.  It does the following trickery:
 *
 * - sets more than one dinfo structure if the driver is a PCI
 *	 driver and we detect more than one instance of the hardware;
 * - substitutes the config table in InstallHints for the passed-in
 *	 configTable if the InstallHints table exists.
 *
 * It returns the number of dinfo structures it actually added.
 */

static int
set_dinfo(
	struct driver_info *dinfo,
	char *bundleName,
	char *configTable,
	char *tableName
)
{
	char *busType, *Ids;
	_pci_slot_info_t	*slot;
	char locationTag[64];
	int ndrivers = 0;
	BOOL detected;
	
	busType = newStringForStringTableKey(configTable, BUS_TYPE);
	if (busType) {
		if (strcmp(busType, PCI_BUS) == 0) {
			Ids = newStringForStringTableKey(configTable, AUTO_DETECT_IDS);
			detected = NO;
			if (Ids) {
				for (slot = PCISlotInfo; slot && slot->pid; slot++) {
					if (testIDs(Ids, slot)) {
						detected = YES;
						sprintf(locationTag, PCI_LOCATION_STRING,
							slot->dev, slot->func, slot->bus);
						_set_dinfo(dinfo++, bundleName,
							configTable, tableName, locationTag);
						ndrivers++;
					}
				}
				free(Ids);
			}
			free(busType);
			if (!detected) {
				_set_dinfo(dinfo++, bundleName, configTable, tableName, NULL);
				ndrivers++;
			}
			return ndrivers;
		}
		free(busType);
	}

	_set_dinfo(dinfo++, bundleName, configTable, tableName, NULL);
	ndrivers++;

	return ndrivers;
}

static int
getMissingDriverList(
	struct driver_info **dinfo_p
)
{
	int i;
	char *bundleName, *tableName;
	char *table;
	char *path = malloc(256);
	char *version;
	int ndrivers = 0;
	struct driver_info *dinfo;
	
	/* Force floppy access. */
	closedir(opendir("fd()/"));
	*dinfo_p = dinfo = (struct driver_info *)
		malloc(sizeof(struct driver_info) * MAX_DRIVERS);
	for(i=0; i<driverMissing; i++) {
		bundleName = missingDrivers[i].bundleName;
		tableName = missingDrivers[i].tableName;
		
		sprintf(path, "%s/%s.config/%s.table",
			usrDevices(), bundleName, tableName);
		if (loadConfigFile(path, &table, YES) == 0) {
			version = newStringForStringTableKey(table, "Version");
			if (strcmp(version, missingDrivers[i].version) == 0) {
				ndrivers += set_dinfo(&dinfo[ndrivers],
					bundleName, table, tableName);
			} else {
				free(table);
			}
		}
	}
	free(path);
	return ndrivers;
}

/*
 * Fills in an array of driver_info structures
 * with the drivers on the current disk that match
 * the interestingFamilies argument.
 * Allocates strings for the .name, .bundle and .version
 * structure elements.
 */
static int
getDriverList(
	struct driver_info **dinfo_p,
	char *interestingFamilies
)
{
	void *dirp;
	int ndrivers = 0;
	struct direct *dp;
	// ecch.. reduce number of function calls to save space
	char *name = malloc(768);			// length 128
	char *tableName = name + 128;		// length 128
	char *path = tableName + 128;		// length 256
	char *configPath = path + 256;		// length 256
	char *cp, *np;
	struct driver_info *dinfo;
	char *configTable;
	char *driver_dir = 0;
	
	verbose("Searching for drivers\n",0);

	/* Iterate over all drivers on this floppy. */
	closedir(opendir("fd()/"));
	driver_dir = usrDevices();
	if (driver_dir == 0)
		driver_dir = "/";
	dirp = opendir(driver_dir);

	if (dirp == 0) {
		error("Couldn't find device driver directory\n");
		goto no_drivers;
	}
	
	*dinfo_p = dinfo = (struct driver_info *)
		malloc(sizeof(struct driver_info) * MAX_DRIVERS);
	for (dp = readdir(dirp); dp != 0; dp = readdir(dirp)) {
		for(cp = dp->d_name, np = name;
			*cp && *cp != '.';)
			*np++ = *cp++;
		*np = '\0';
		// cp points at '.' or '\0'
			
		if (strcmp(dp->d_name, "System.config") == 0 ||
			strcmp(cp, ".config") != 0)
			continue;

		{
		void *tdirp;
		struct direct *tdp;

		sprintf(path, "%s/%s", driver_dir, dp->d_name);
		tdirp = opendir(path);
		for (tdp = readdir(tdirp); tdp != 0; tdp = readdir(tdirp)) {
			for(cp = tdp->d_name, np = tableName;
				*cp && *cp != '.';)
				*np++ = *cp++;
			*np = '\0';
			// cp points at '.' or '\0'
			if (*cp && strcmp(cp, ".table") == 0) {
				// Skip Instance%d.table and Default.table
				if (strncmp(tdp->d_name, "Default", sizeof("Default")-1) &&
					strncmp(tdp->d_name, "Instance", sizeof("Instance")-1)) {
					// We found a bonus instance table within this bundle

					/* Load driver name from Localizable.strings file. */
					configTable = NULL;
					sprintf(configPath, "%s/%s", path, tdp->d_name);
					loadConfigFile(configPath, &configTable, YES);
			
					if (isInteresting(name, configTable,
													interestingFamilies)) {
						ndrivers += set_dinfo(&dinfo[ndrivers],
							name, configTable, tableName);
					} else {
						free(configTable);
					}
			
					if (ndrivers == MAX_DRIVERS)
						break;
				}
			
			}
		}
		closedir(tdirp);
		}
		
		if (ndrivers == MAX_DRIVERS)
			break;
						
		/* Load driver name from Localizable.strings file. */
		configTable = NULL;
		loadConfigDir(name, NO, &configTable, YES);

		if (isInteresting(name, configTable, interestingFamilies)) {
			ndrivers += set_dinfo(&dinfo[ndrivers],
				name, configTable, NULL);
		} else {
			free(configTable);
		}

		if (ndrivers == MAX_DRIVERS)
			break;
	}
no_drivers:
	free(name);
//	  free(tableName);
//	  free(path); free(configPath);
	closedir(dirp);
	if (ndrivers)
		sort_names(*dinfo_p, ndrivers);
	return ndrivers;
}

static void
freeDriverList(
	struct driver_info *dinfo,
	int ndrivers
)
{
	int i;
	for (i = 0; i < ndrivers; i++) {
		free(dinfo[i].name);
		free(dinfo[i].bundle);
		free(dinfo[i].version);
		free(dinfo[i].configTable);
		free(dinfo[i].tableName);
		free(dinfo[i].locationTag);
	}
	free(dinfo);
}



int
loadBootDrivers(
	BOOL		askFirst,
	int 		numberOfPrompts
)
{
	int old_kerndev;
	int ndrivers, iteration, i;
	BOOL newDisk;
	struct driver_info *drivers = NULL;
	int wasLoadingMissingDrivers = driverMissing;
	char familyNames[32], *families;
	char askKey[32];

	old_kerndev = kernBootStruct->kernDev;

	for (iteration = 1, newDisk = YES, ndrivers = 0;
		askFirst || (iteration <= numberOfPrompts); ) {
		
		if (askAboutMissingDrivers()) {
			newDisk = YES;
			flushdev();
		} else {
			/* Ask if we want to load drivers */
			sprintf(askKey, "DRIVER_ASK_%d", iteration);
			if (askFirst || getBoolForKey(askKey)) {
				newDisk = YES;
				flushdev();
				if (askWhetherToLoadMoreDrivers() == 0)
					break;
			}
		}

dont_ask:		
		sprintf(familyNames, "DRIVER_FAMILIES_%d", iteration);
		families = newStringForKey(familyNames);
		
		if (newDisk) {
			if (drivers != NULL)
				freeDriverList(drivers, ndrivers);
			if (driverMissing)
				ndrivers = getMissingDriverList(&drivers);
			else
				ndrivers = getDriverList(&drivers, 0);
			newDisk = NO;
		}

		if (ndrivers) {
			/* Mark "interesting" drivers. */
			for (i=0; i<ndrivers; i++)
				if (isInteresting(drivers[i].name,
					drivers[i].configTable, families))
						drivers[i].flags = DRIVER_FLAG_INTERESTING;
				else
						drivers[i].flags = DRIVER_FLAG_NONE;

			i = pickDrivers(drivers, ndrivers,
						driverMissing, askFirst ? 0 : iteration);
			if (i == 0) {
				/* We loaded a driver */
				if (driverMissing == 0 && !getBoolForKey(askKey))
					iteration++;
			} else if (i == -1) {
				/*
				 * User wants to use another disk.
				 */
				newDisk = YES;
				flushdev();
				goto dont_ask;
			} else if (i == -2) {
				/*
				 * User wants to quit.
				 */
				newDisk = YES;
				flushdev();
			}

			if (wasLoadingMissingDrivers && (driverMissing == 0)) {
				/* We loaded all missing drivers; we're finished. */
				break;
			}
		} else {
			printf(" \n");
			/* Localized text */
			localPrintf("No Drivers On This Disk");
			newDisk = YES;
			flushdev();
			while (getc() != '\r')
				continue;
			printf("\n");
		}
	}
	flushdev();
	kernBootStruct->kernDev = old_kerndev;
	freeDriverList(drivers, ndrivers);
	return 0;
}

static void
sort_names(
	struct driver_info *dinfo,
	int n
)
{
	register struct driver_info temp;
	register int gap, i, j;
	
	gap = 1;
	do (gap = 3 * gap + 1); while (gap <= n);
	for (gap /= 3; gap > 0; gap /= 3)
		for (i = gap; i < n; i++) {
			temp = dinfo[i];
			for (j = i - gap;
				(j >= 0) && (strcmp(dinfo[j].name,temp.name) > 0);
				j -= gap) {
					dinfo[j + gap] = dinfo[j];
			}
			dinfo[j + gap] = temp;
		}
}

/*
 * Record the fact that a driver is missing.
 * Assumes that all strings are malloced.
 */
void
driverIsMissing(
	char *bundleName,
	char *version,
	char *longName,
	char *tableName
)
{
	struct _missingDriver *dp = &missingDrivers[driverMissing++];

	dp->bundleName = bundleName;
	dp->version = version;
	dp->longName = longName ? longName : newString(bundleName);
	dp->tableName = tableName ? tableName : newString("Default");
}

/*
 * You loaded a (possibly) missing driver.
 * If the name passed in matches one of the missing drivers,
 * it's removed from the list and the original string is freed.
 */
void
loadedPossiblyMissingDriver(char *name)
{
	int i;
	for (i=0; i < driverMissing; i++) {
		if (strcmp(name, missingDrivers[i].bundleName) == 0) {
			free(missingDrivers[i].bundleName);
			free(missingDrivers[i].longName);
			free(missingDrivers[i].version);
			free(missingDrivers[i].tableName);
			driverMissing--;
			for(; i < driverMissing; i++) {
				missingDrivers[i] = missingDrivers[i+1];
			}
			return;
		}
	}
}

void
addToLoadedDriverList(
	char *bundleName,
	char *longName,
	char *configTable,
	char *tableName,
	int flags
)
{
	if (loaded_drivers == 0) {
		loaded_drivers = (struct driver_info *)
			malloc(sizeof(struct driver_info) * MAX_DRIVERS);
	}
	loaded_drivers[num_loaded].name = longName;
	loaded_drivers[num_loaded].bundle = bundleName;
	if ((loaded_drivers[num_loaded].version =
		newStringForStringTableKey(configTable, "Version")) == NULL)
			loaded_drivers[num_loaded].version = "1.0";
	loaded_drivers[num_loaded].flags = flags;
	loaded_drivers[num_loaded].configTable = configTable;
	num_loaded++;
}

BOOL
isInteresting(
	char *name,
	char *configTable,
	char *interestingFamilies
)
{
	char *familyVal, *listVal, *listElt;
	int familyLen, listLen;
	BOOL freeTable = NO, found = NO;
	
	if (interestingFamilies == 0)
		return YES;
		
	if (configTable == 0) {
		if (loadConfigDir(name, NO, &configTable, YES) < 0)
			return NO;
		freeTable = YES;
	}
	if (getValueForStringTableKey(configTable, "Family",
									&familyVal, &familyLen) == NO) {
		if (freeTable)
			free(configTable);
		return NO;
	}
	listVal = interestingFamilies;
	listLen = strlen(listVal);
	while ((found == NO) &&
		   (listElt = (char *)newStringFromList(&listVal, &listLen))) {
		if (strlen(listElt) == familyLen &&
			strncmp(listElt, familyVal, familyLen) == 0) {
			found = YES;
		}
		free(listElt);
	}
	if (freeTable)
		free(configTable);
	return found;
}

void
driverWasLoaded(char *name, char *configTable, char *tableName)
{
	loadedPossiblyMissingDriver(name);
	addToLoadedDriverList(newString(name),
			(char *)bundleLongName(name, tableName), configTable,
			tableName,
			isInteresting(name, configTable, LoadableFamilies) ?
			DRIVER_FLAG_INTERESTING : DRIVER_FLAG_NONE);
}



