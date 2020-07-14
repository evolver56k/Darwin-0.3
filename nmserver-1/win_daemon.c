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
 * Copyright (c) 1995, Apple Computer, Inc.
 * All Rights Reserved.
 *
 * win_daemon.c created by gvdl 12 July 1996
 */

#if defined(WIN32)

#include <windows.h>
#include <winnt-pdo.h>

#define APP_NAME "nmserver"

#define MY_SERVICE_NAME "Apple_Netname_Server"

#define APP_TITLE "Apple Netname Server"

#define APP_DEPENDENCIES "Apple_Mach_Daemon\0\0"

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   service.c
//
//  PURPOSE:  Implements functions required by all services
//            windows.
//
//  FUNCTIONS:
//    main(int argc, char **argv);
//    service_ctrl(DWORD dwCtrlCode);
//    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
//    CmdInstallService();
//    CmdRemoveService();
//    CmdDebugService(int argc, char **argv);
//    ControlHandler ( DWORD dwCtrlType );
//    GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
//
//  COMMENTS:
//
//  AUTHOR: Craig Link - Microsoft Developer Support
//


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);	// nmserver.c
VOID ServiceStop();					// nmserver.c

MS_BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

void AddToMessageLog(LPTSTR lpszMsg);


// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
MS_BOOL                 bDebug = FALSE;
TCHAR                   szErr[256];

// internal function prototypes
static VOID WINAPI service_ctrl(DWORD dwCtrlCode);
static VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
static VOID CmdDeleteService();
static DWORD CmdInstallService();
static VOID CmdRemoveService();
static VOID CmdDebugService(int argc, char **argv);
static MS_BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
static LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );

//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
static VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_STOP:
            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ServiceStop();
            break;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}



//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
static void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(MY_SERVICE_NAME), service_ctrl);

    if (!sshStatusHandle)
        goto cleanup;

    // SERVICE_STATUS members that don't change in example
    //
    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;


    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;


    ServiceStart( dwArgc, lpszArgv );

cleanup:

    // try to report the stopped status to the service control manager.
    //
    if (sshStatusHandle)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            dwErr,
                            0);

    return;
}



//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
MS_BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    MS_BOOL fResult = TRUE;


    if ( !bDebug ) // when debugging we don't report to the SCM
    {
        if (dwCurrentState == SERVICE_START_PENDING)
            ssStatus.dwControlsAccepted = 0;
        else
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;


        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            AddToMessageLog(TEXT("SetServiceStatus"));
        }
    }
    return fResult;
}



//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(LPTSTR lpszMsg)
{
    TCHAR   szMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[2];


    if ( !bDebug )
    {
        dwErr = GetLastError();

        // Use event logging to log the error.
        //
        hEventSource = RegisterEventSource(NULL, TEXT(MY_SERVICE_NAME));

        _stprintf(szMsg, TEXT("%s error: %ld"), TEXT(MY_SERVICE_NAME), dwErr);
        lpszStrings[0] = szMsg;
        lpszStrings[1] = lpszMsg;

        if (hEventSource != NULL) {
            ReportEvent(hEventSource, // handle of event source
                EVENTLOG_ERROR_TYPE,  // event type
                0,                    // event category
                0,                    // event ID
                NULL,                 // current user's SID
                2,                    // strings in lpszStrings
                0,                    // no bytes of raw data
                lpszStrings,          // array of error strings
                NULL);                // no raw data

            (VOID) DeregisterEventSource(hEventSource);
        }
    }
}




///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//

static void CmdDeleteService(SC_HANDLE schService, MS_BOOL verbose)
{
    // try to stop the service
    if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
    {
	if (verbose)
	    _tprintf(TEXT("Stopping %s."), TEXT(APP_TITLE));
	Sleep( 1000 );

	while ( QueryServiceStatus( schService, &ssStatus ) )
	{
	    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
	    {
		if (verbose)
		    _tprintf(TEXT("."));
		Sleep( 1000 );
	    }
	    else
		break;
	}


    }

    if (verbose)
    {
	if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
	    _tprintf(TEXT("\n%s stopped.\n"), TEXT(APP_TITLE) );
	else
	    _tprintf(TEXT("\n%s failed to stop.\n"), TEXT(APP_TITLE) );
    }

    // now remove the service
    if (DeleteService(schService))
    {
	if (verbose)
	    _tprintf(TEXT("%s removed.\n"), TEXT(APP_TITLE) );
    }
    else if (verbose)
	_tprintf(TEXT("DeleteService failed - %s\n"),
	    GetLastErrorText(szErr,256));
}

//
//  FUNCTION: CmdRemoveService()
//
//  PURPOSE: Stops and removes the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
static void CmdRemoveService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( !schSCManager )
    {
	_tprintf(TEXT("OpenSCManager failed - %s\n"),
	    GetLastErrorText(szErr,256));
	return;
    }

    schService = OpenService(schSCManager,
	TEXT(MY_SERVICE_NAME), SERVICE_ALL_ACCESS);
    if ( !schService )
    {
    	_tprintf(TEXT("OpenService failed - %s\n"),
	    GetLastErrorText(szErr,256));
	goto abort;
    }

    CmdDeleteService(schService, TRUE /* verbose */);
    
    CloseServiceHandle(schService);

abort:
    CloseServiceHandle(schSCManager);
}




//
//  FUNCTION: CmdInstallService()
//
//  PURPOSE: Installs the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
static DWORD CmdInstallService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
    DWORD ret = ERROR_SUCCESS;

    TCHAR szPath[512];

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
    {
	ret = GetLastError();
        _tprintf(TEXT("Unable to install %s - %s\n"),
	    TEXT(APP_TITLE), GetLastErrorText(szErr, 256));
        return ret;
    }

    schSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if ( !schSCManager )
    {
	ret = GetLastError();
        _tprintf(TEXT("OpenSCManager failed - %s\n"),
	    GetLastErrorText(szErr,256));
	return ret;
    }

    // If the old service is present, just return ERROR_SERVICE_ALREADY_RUNNING.
    // This is because of an apparent NT bug which shows up while running the installer:
    // if we delete the service, it often disappears after the next reboot!
    schService = OpenService(schSCManager, TEXT(MY_SERVICE_NAME),
    SERVICE_ALL_ACCESS);
    if ( schService )
    {
        ret = ERROR_SERVICE_ALREADY_RUNNING;
        _tprintf(TEXT("Not attempting to overwrite running service\nThe new version will be present on reboot\n"));
        return ret;
    }

    schService = CreateService(
	schSCManager,               // SCManager database
	TEXT(MY_SERVICE_NAME),      // name of service
	TEXT(APP_TITLE),	    // name to display
	SERVICE_ALL_ACCESS,         // desired access
	SERVICE_WIN32_OWN_PROCESS,  // service type
	SERVICE_AUTO_START,         // start type
	SERVICE_ERROR_NORMAL,       // error control type
	szPath,                     // service's binary
	NULL,                       // no load ordering group
	NULL,                       // no tag identifier
	TEXT(APP_DEPENDENCIES),     // dependencies
	NULL,                       // LocalSystem account
	NULL);                      // no password

    if ( schService )
    {
	_tprintf(TEXT("%s installed.\n"), TEXT(APP_TITLE) );
	CloseServiceHandle(schService);
    }
    else
    {
	ret = GetLastError();
	_tprintf(TEXT("CreateService failed - %s\n"),
	    GetLastErrorText(szErr, 256));
    }

    CloseServiceHandle(schSCManager);
    return ret;
}



///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//


//
//  FUNCTION: ControlHandler ( DWORD dwCtrlType )
//
//  PURPOSE: Handled console control events
//
//  PARAMETERS:
//    dwCtrlType - type of control event
//
//  RETURN VALUE:
//    True - handled
//    False - unhandled
//
//  COMMENTS:
//
static MS_BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(TEXT("Stopping %s.\n"), TEXT(APP_TITLE));
            ServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

//
//  FUNCTION: CmdDebugService(int argc, char ** argv)
//
//  PURPOSE: Runs the service as a console application
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//

// In nmserver.c: The non-service main...
extern void other_win32_main(int argc, char **argv);
static void CmdDebugService(int argc, char ** argv)
{
    DWORD dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(TEXT("Debugging %s.\n"), TEXT(APP_TITLE));

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    other_win32_main( dwArgc, lpszArgv );
}


//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}



//
// FUNCTION: WinMain
//
// PURPOSE: Allows nmserver to run without console windows on
//	    Win95.
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {
    return main(__argc, __argv);
}


//
//  FUNCTION: main
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    main() either performs the command line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//

void is_there_anyone_out_there(void);

int main(int argc, char **argv) {
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(MY_SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION) service_main },
        { NULL, NULL }
    };

    if ( (argc > 1) &&
         ((*argv[1] == '-') || (*argv[1] == '/')) )
    {
        if ( _stricmp( "install", argv[1]+1 ) == 0 )
            exit(CmdInstallService());
        else if ( _stricmp( "remove", argv[1]+1 ) == 0 )
            CmdRemoveService();
        else if ( 'd' == argv[1][1] )
        {
            bDebug = TRUE;
            CmdDebugService(argc, argv);
        }
        else
            goto dispatch;
        exit(0);
    }

    // if it doesn't match any of the above parameters
    // the service control manager may be starting the service
    // so we must call StartServiceCtrlDispatcher
dispatch:
    // this is just to be friendly
    printf( "%s -install          to install the service\n", APP_NAME );
    printf( "%s -remove           to remove the service\n", APP_NAME );
    printf( "%s -d <params>   to run as a console app for debugging\n", APP_NAME );
    printf( "\nStartServiceCtrlDispatcher being called.\n" );
    printf( "This may take several seconds.  Please wait.\n" );

    if (StartServiceCtrlDispatcher(dispatchTable)) {
    	// Cool, we're done...
	exit(0);
    }
    else if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError()) {
	// Windoze 95 strikes again!
	is_there_anyone_out_there();
	ServiceStart(argc, argv);
	exit(0);
    }
    else {
	AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
	exit(1);
    }
}

/*
 * Use Mike Paquette's interlock trick.
 */

#define MY_MAPPING_NAME MY_SERVICE_NAME

#define ALREADY_RUNNING "The Apple Netname Server is already running."

#define WANNA_QUIT "The Apple Netname Server is already running.  Do you want to terminate it?"

#define QUIT_WARNING "Terminating the Netname Server will prevent any OPENSTEP applications from launching and will hang any running OPENSTEP applications.  Proceed anyway?"

#define CANT_NUKE_WARNING "Can't terminate the current Netname Server."

#define CANT_WRITE_PROCESS_ID "Cannot write process ID to shared memory."

#define GENERIC_PROBLEM "Error creating mapping."

void
is_there_anyone_out_there (void)
{
    HANDLE mapping, process;
    unsigned long *ppid;
    int r;

    /*
     * Try to generate a unique app-specific mapping in the system paging file.
     * This should fail if there is another mapping with the same name, thereby
     * guaranteeing that only one application instance is running at any given
     * time.  Known to work for NT and 95.
     */
    mapping = CreateFileMapping((HANDLE)0xffffffff, NULL, PAGE_READWRITE, 0,
        32, MY_MAPPING_NAME);

    if (mapping) {
        if (ERROR_ALREADY_EXISTS == GetLastError()) {
            /* There's another app already running.  Offer to nuke it. */
            CloseHandle(mapping);
            mapping = OpenFileMapping(FILE_MAP_READ, FALSE, MY_MAPPING_NAME);
            ppid = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(*ppid));
            if (NULL == ppid) {
                /* We can't get the other instance's pid, so just die. */
                MessageBox(NULL, ALREADY_RUNNING, APP_TITLE, MB_OK);
                ExitProcess(1);
            }

            /* Offer to nuke the other running app. */
            r = MessageBox(NULL, WANNA_QUIT, APP_TITLE,
                MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION);
            if (IDYES != r)
                ExitProcess(1);

            /* Warn about possible data loss. */
            r = MessageBox(NULL, QUIT_WARNING, APP_TITLE,
                MB_OKCANCEL|MB_DEFBUTTON2|MB_ICONSTOP);
            if (IDOK != r)
                ExitProcess(1);

            /*
             * Open and terminate the process.  Mimics the behavior of PVIEW
             * and KILL.
             *
             * Caveats:  DLLs used by the application will not receive notice of
             * app termination.  Particularly weird DLLs that rely on app
             * termination notification (e.g., they hold app state globally)
             * may be left in an unstable state. This doesn't seem to be a problem
             * with the Microsoft supplied system DLLs.
             */
            process = OpenProcess(PROCESS_TERMINATE, TRUE, *ppid);
            if (!process || !TerminateProcess(process, 0)) {
                MessageBox(NULL, CANT_NUKE_WARNING, APP_TITLE, MB_OK);
            }
            ExitProcess(1);
        } else {
            /* Create a file mapping. */
            ppid = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(*ppid));
            if (NULL == ppid) {
                MessageBox(NULL, CANT_WRITE_PROCESS_ID, APP_TITLE, MB_OK);
                ExitProcess(1);
            }
            *ppid = GetCurrentProcessId();
        }
    } else {
        /* Some other error in setting up the mapping. */
        MessageBox(NULL, GENERIC_PROBLEM, APP_TITLE, MB_OK);
        ExitProcess(1);
    }
}

#endif /* defined(WIN32) */
