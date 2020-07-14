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
#define SAIO_INTERNAL 1
#import "saio_types.h"

#ifdef i386			// processor specific -- probabably not totally correct

extern void  real_to_prot(void);
extern void  prot_to_real(void);
extern void  bios(biosBuf_t *bb);
extern BOOL eisa_present(void);
extern int  bgetc(void);
extern int  biosread(int dev, int cyl, int head, int sec, int num);
extern void  setA20(void);
extern void  turnOffFloppy(void);
#endif

#ifdef ppc
extern int openmem(char *buf, int len);
extern void set_video_mode(unsigned int mode);
#endif

extern void  halt(void);
extern void  putc(int ch);
extern int  getc(void);
extern int  readKeyboardStatus(void);
extern unsigned int  time18(void);
//extern unsigned int  get_diskinfo(int dev);
extern int APMPresent(void);
extern int APMConnect32(void);
extern int  memsize(int i);
extern void  video_mode(int mode);
extern char *usrDevices (void);
extern volatile void stop(char *message);
extern int  open(char *str, int how);
extern int  close(int fdesc);
extern int  file_size(int fdesc);
extern int  read(int fdesc, char *buf, int count);
extern int  b_lseek(int fdesc, unsigned int addr, int ptr);
extern int  tell(int fdesc);
extern void  flushdev(void);
extern char *usrDevices(void);
extern struct dirstuff * opendir(char *path);
extern int  closedir(struct dirstuff *dirp);
extern struct direct * readdir(struct dirstuff *dirp);
extern void  putchar(int ch);
extern int  getchar(void);
extern int  gets(char *buf, int len);
extern char * newStringFromList(char **list, int *size);
extern int  stringLength(char *table, int compress);
extern BOOL  getValueForStringTableKey(char *table, char *key, char **val, int *size);
extern char * newStringForStringTableKey(char *table, char *key);
extern char * newStringForKey(char *key);
extern BOOL  getValueForBootKey(char *line, char *match, char **matchval, int *len);
extern BOOL  getValueForKey(char *key, char **val, int *size);
extern BOOL  getBoolForKey(char *key);
extern BOOL  getIntForKey(char *key, int *val);
extern char * loadLocalizableStrings(char *name, char *tableName);
extern char * bundleLongName(char *bundleName, char *tableName);
extern int  loadConfigFile( char *configFile, char **table, BOOL allocTable);
extern int  loadConfigDir(
    char *bundleName,
    BOOL useDefault,
    char **table,
    BOOL allocTable
);
extern int  loadSystemConfig(char *which, int size);
extern int  loadOtherConfigs(int useDefault);
extern void addConfig(char *config);

extern int  loadmacho(
    struct mach_header *head,
    int dev,
    int io,
    entry_t *entry,
    char **addr,
    int *size,
    int file_offset
);
extern int  loadprog(
    int dev,
    int fd,
    struct mach_header *headOut,
    entry_t *entry,
    char **addr,
    int *size
);
extern void  removeLinkEditSegment(struct mach_header *mhp);
extern void  driverIsMissing(
    char *name,
    char *version,
    char *longName,
    char *tableName
);
extern int  driversAreMissing(void);

#import "load.h"

extern int  pickDrivers(
    struct driver_info *dinfo,
    int ndrivers,
    int autoLoad,
    int instruction
);
extern int  loadBootDrivers(BOOL, int);
extern BOOL isInteresting(
    char *name, char *configTable, char *interestingFamilies);
extern void  driverWasLoaded(char *name, char *configTable, char *tableName);
extern int  localVPrintf(const char *format, va_list ap);
extern int  loadLanguageFiles(void);
extern char * getLanguage(void);
extern char * setLanguage(char *new);
extern int  errorV(const char *format, va_list ap);
extern int  getErrors(void);
extern int  setErrors(int new);
extern void  sleep(int n);
extern int  chooseSimple( char **strings, int nstrings, int min, int max );
extern int  chooseDriverFromList(
    char *title,
    char *message,
    struct driver_info *drivers,
    int nstrings,
    char *footMessage,
    char *moreMessage,
    char *quit1Message,
    char *quit2Message
);
extern void  clearScreen(void);
extern void  copyImage(const struct bitmap *bitmap, int x, int y);
extern void  clearRect(int x, int y, int w, int h, int c);
extern struct bitmap * loadBitmap(char *filename);
extern void set_video_mode(unsigned int mode);
extern int reallyPrint(const char *fmt, ...);
extern char *newString(char *oldString);
extern int currentdev(void);
extern int switchdev(int dev);


