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
 * Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved.
 *
 * kmLocalized.m -- strings for localized graphics
 */

#import <driverkit/i386/driverTypesPrivate.h>
#import <bsd/dev/kmreg_com.h>
#import <bsd/dev/i386/km.h>
#import <bsd/dev/i386/BasicConsole.h>

const char *kmLocalizedStrings[][L_NUM_LANGUAGE] = {
{
"Restarting the computer...\n",			// L_ENGLISH
"Redemarrage en cours...\n",			// L_FRENCH
"Starte neu...\n",				// L_GERMAN
"Reinicializando...\n",				// L_SPANISH
"Riavvio...\n",					// L_ITALIAN
"Startar om...\n",				// L_SWEDISH
NULL,						// L_JAPANESE
},

{ 
"Please wait until it's safe\n"			// L_ENGLISH
"to turn off the computer.\n",
"Veuillez patienter avant\n"			// L_FRENCH
"d'eteindre votre ordinateur.\n",
"Bitte warten Sie, bis Sie Ihren Computer\n"	// L_GERMAN
"sicher ausschalten koennen.\n",
"Espere hasta que sea seguro\n"			// L_SPANISH
"apagar el ordenador.\n",
"Prima di spegnere il computer,\n"		// L_ITALIAN
"attendi la conferma.\n",
"Vanta tills det ar sakert att stanga av datorn.\n", // L_SWEDISH
NULL,						// L_JAPANESE
},

{
"It's safe to turn off the computer.\n",	// L_ENGLISH
"Vous pouvez maintenant eteindre\n"
"votre ordinateur en toute securite.\n",	// L_FRENCH
"Jetzt koennen Sie Ihren Computer\n"
"sicher ausschalten.\n",			// L_GERMAN
"Ahora es seguro apagar el ordenador.\n",	// L_SPANISH
"Ora puoi spegnere il computer.\n",		// L_ITALIAN
"Nu ar det sakert att stanga av datorn.\n",	// L_SWEDISH
NULL,						// L_JAPANESE
},

{
"Please wait...",				// L_ENGLISH
NULL,						// L_FRENCH
NULL,						// L_GERMAN
NULL,						// L_SPANISH
NULL,						// L_ITALIAN
NULL,						// L_SWEDISH
NULL,						// L_JAPANESE
},

{ NULL }					// END

};

const char *
kmLocalizeString(
    const char *str
)
{
    int i, lang;
    const char *newStr;
    
    lang = glLanguage;
    if (lang < 0 || lang >= L_NUM_LANGUAGE)
	lang = L_ENGLISH;
	
    for (i = 0; kmLocalizedStrings[i][0] != NULL; i++) {
	if (strcmp(kmLocalizedStrings[i][0], str) == 0) {
	    if ((newStr = kmLocalizedStrings[i][lang]) != NULL)
		return newStr;
	    break;
	}
    }
    return str;
}
