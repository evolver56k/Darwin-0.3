/* 
 * tclLoadNext.c --
 *
 *	This procedure provides a version of the TclLoadFile that
 *	works with Apple's dyld dynamic loading.  This file provided
 *	by Wilfredo Sanchez.
 *
 * Copyright (c) 1995 Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tclLoadRhapsody.c,v 1.1.1.1 1999/04/23 01:58:28 wsanchez Exp $
 */

#include "tclInt.h"
#include <mach-o/dyld.h>

/*
 *----------------------------------------------------------------------
 *
 * TclLoadFile --
 *
 *	Dynamically loads a binary code file into memory and returns
 *	the addresses of two procedures within that file, if they
 *	are defined.
 *
 * Results:
 *	A standard Tcl completion code.  If an error occurs, an error
 *	message is left in interp->result.  *proc1Ptr and *proc2Ptr
 *	are filled in with the addresses of the symbols given by
 *	*sym1 and *sym2, or NULL if those symbols can't be found.
 *
 * Side effects:
 *	New code suddenly appears in memory.
 *
 *----------------------------------------------------------------------
 */

int
TclLoadFile(interp, fileName, sym1, sym2, proc1Ptr, proc2Ptr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    char *fileName;		/* Name of the file containing the desired
				 * code. */
    char *sym1, *sym2;		/* Names of two procedures to look up in
				 * the file's symbol table. */
    Tcl_PackageInitProc **proc1Ptr, **proc2Ptr;
				/* Where to return the addresses corresponding
				 * to sym1 and sym2. */
{
  NSObjectFileImageReturnCode err;
  NSObjectFileImage           image;
  NSModule                    module;
  NSSymbol                    symbol;
  char                        *name;

  if ((err=NSCreateObjectFileImageFromFile(fileName, &image)) != NSObjectFileImageSuccess) {
    switch (err) {
      case NSObjectFileImageFailure:
        interp->result = "dyld: general failure"; break;
      case NSObjectFileImageInappropriateFile:
        interp->result = "dyld: inappropriate Mach-O file"; break;
      case NSObjectFileImageArch:
        interp->result = "dyld: inappropriate Mach-O architecture"; break;
      case NSObjectFileImageFormat:
        interp->result = "dyld: invalid Mach-O file format"; break;
      case NSObjectFileImageAccess:
        interp->result = "dyld: permission denied"; break;
      default:
        interp->result = "dyld: unknown failure"; break;
    }
    return TCL_ERROR;
  }

  module = NSLinkModule(image, fileName, TRUE);

  if (module == NULL) {
    interp->result = "dyld: falied to link module";
    return TCL_ERROR;
  }

  name = (char*)malloc(sizeof(char)*(strlen(sym1)+2));
  sprintf(name, "_%s", sym1);
  symbol = NSLookupAndBindSymbol(name);
  free(name);
  *proc1Ptr = NSAddressOfSymbol(symbol);

  name = (char*)malloc(sizeof(char)*(strlen(sym2)+2));
  sprintf(name, "_%s", sym2);
  symbol = NSLookupAndBindSymbol(name);
  free(name);
  *proc2Ptr = NSAddressOfSymbol(symbol);

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *	If the "load" command is invoked without providing a package
 *	name, this procedure is invoked to try to figure it out.
 *
 * Results:
 *	Always returns 0 to indicate that we couldn't figure out a
 *	package name;  generic code will then try to guess the package
 *	from the file name.  A return value of 1 would have meant that
 *	we figured out the package name and put it in bufPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(fileName, bufPtr)
    char *fileName;		/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr;	/* Initialized empty dstring.  Append
				 * package name to this if possible. */
{
    return 0;
}
