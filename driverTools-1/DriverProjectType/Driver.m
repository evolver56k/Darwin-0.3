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
#import <ProjectBuilder/PBProjectType.h>
#import <ProjectBuilder/PBProjectFiles.h>

#define EXPANSION_DELIMITER	'$'

@interface LKSProgressPanel : NSObject
{
}

// the delegate is responsible to provide UI feedback
+ progressPanel;
@end


@implementation LKSProgressPanel
+ progressPanel;
{
    return nil;
}

@end


@interface DriverProjectType : PBProjectType
{
}

@end

@implementation DriverProjectType

static void createLKS(PBProject *project)
{
    NSString *lksProjName = [project projectName];
    NSString  *lksProjectDir;
    PBProject *lksProj;
    PBProjectType *lksProjType;

    NS_DURING
    {
        lksProjType = [PBProjectType projectTypeNamed: @"KernelServer"];
        if (!lksProjType)
            NSLog(@"Can't find project type: KernelServer");

        /* Ignore the progress panel requests of the KernelServer project */
        [lksProjType setDelegate: [LKSProgressPanel class]];
        lksProj = [lksProjType instantiateProjectNamed: lksProjName
                                           inDirectory: [project projectDir]
                                appendProjectExtension: YES];
    }
    NS_HANDLER
        lksProj = nil;
    NS_ENDHANDLER

    if (!lksProj)
        return;

    lksProjectDir = [lksProj projectDir];
    [lksProj setLanguageName: [project languageName]];
    if ([lksProjType maintainsFile: @"Makefile"])
    {
        NSString *makeFileDir =
            [[NSSearchPathForDirectoriesInDomains(NSDeveloperDirectory, 		NSSystemDomainMask, NO) objectAtIndex: 0]
                    stringByAppendingPathComponent: @"/Makefiles/pb_makefiles"];
        NSString *tmpltPre  = [makeFileDir
            stringByAppendingPathComponent: @"Makefile.preamble.template"];
        NSString *tmpltPost = [makeFileDir
            stringByAppendingPathComponent: @"Makefile.postamble.template"];
        NSString *lksPre  = [lksProjectDir
            stringByAppendingPathComponent: @"Makefile.preamble"];
        NSString *lksPost = [lksProjectDir
            stringByAppendingPathComponent: @"Makefile.postamble"];
        NSFileManager *fileManager = [NSFileManager defaultManager];

        if ([fileManager fileExistsAtPath: tmpltPre]
        && ![fileManager fileExistsAtPath: lksPre])
            [fileManager copyPath: tmpltPre toPath: lksPre handler: nil];

        if ([fileManager fileExistsAtPath: tmpltPost]
        && ![fileManager fileExistsAtPath: lksPost])
            [fileManager copyPath: tmpltPost toPath: lksPost handler: nil];
    }
    [lksProj saveProjectFiles];

    /* Now save the KernelServer project and include in the current project */
    [project addFile: [lksProjectDir lastPathComponent] key: @"SUBPROJECTS"];
}    

static BOOL expandVariablesInTemplateFile
     (NSString *in, NSString *out, NSDictionary *dictionary)
{
    FILE *input, *output;
    int ch;
    char buffer[1024];

    if (!(input = fopen([in cString], "r")))
       return NO;
    if (!(output = fopen([out cString], "w")))
    {
        fclose(input);
        return NO;
    }

    while ((ch = getc(input)) != EOF)
    {
        if (EXPANSION_DELIMITER != ch)
            putc(ch, output);
        else
        {
            ch = getc(input);
            if (EXPANSION_DELIMITER == ch)
                while ('\n' != ch && EOF != ch)
                    ch = getc(input);
            else
            {
                NSString *value, *key;
                char *ptr = buffer;

                buffer[0] = '\0';
                while (EXPANSION_DELIMITER != ch)
                {
                   *ptr++ = ch;
                   ch = getc(input);
                }
                *ptr++ = '\0';

                key = [NSString stringWithCString: buffer];
                if (nil == (value = [dictionary objectForKey:key]))
                   NSLog(@"expanding %@: variable has no value: %@", in, key);
                else
                   fprintf(output, "%s", [value cString]);
            }
        }
    }
    fclose(input);
    fclose(output);
    return YES;
}

- (void) customizeNewProject: (PBProject *) project
{
    NSString *projectDir = [project projectDir];
    NSMutableDictionary *variables;
    NSString *inFile, *outFile;

    /* First create a valid Kernel Loadable project */
    createLKS(project);
   
    /* Now customise the current project */
    /* Create the dictionary that provides interesting info */
    variables = [NSMutableDictionary dictionary];
    [variables setObject:[project projectName] forKey:@"PROJECTNAME"];
    [variables setObject:[[NSDate date] descriptionWithCalendarFormat:@"%B %e, %Y" timeZone:nil locale:nil] forKey:@"DATE"];
    [variables setObject:NSFullUserName() forKey:@"USERNAME"];

    outFile =
        [projectDir stringByAppendingPathComponent: @"Default.table"];
    inFile =
        [projectDir stringByAppendingPathComponent: @"Default.table.template"];
    expandVariablesInTemplateFile(inFile, outFile, variables);
    [project addFile: @"Default.table" key: @"OTHER_RESOURCES"];
    [[NSFileManager defaultManager] removeFileAtPath: inFile handler:nil];    

    outFile =
        [projectDir stringByAppendingPathComponent: @"Localizable.strings"];
    inFile =
    [projectDir stringByAppendingPathComponent: @"Localizable.strings.template"];
    expandVariablesInTemplateFile(inFile, outFile, variables);
    [project addFile: @"Localizable.strings" key: @"OTHER_RESOURCES"];
    [[NSFileManager defaultManager] removeFileAtPath: inFile handler:nil];    
}

@end
