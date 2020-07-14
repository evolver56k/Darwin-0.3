/*
 * File: ntpdll.c
 * Purpose: DLL based helper functions for InstallShield SDK
 *
 */

/* includes */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../libntp/log.h"


// ---------------------------------------------------------------------------------------
SC_HANDLE schService;
SC_HANDLE ntpService;

// ---------------------------------------------------------------------------------------

LONG APIENTRY CreateNTPService(HWND hwnd, LPLONG lpIValue, LPSTR serviceexe)
{
  LPCTSTR serviceName = "NetworkTimeProtocol";
  LPCTSTR lpszBinaryPathName = serviceexe;
  LPTSTR  lpszRootPathName="?:\\";
  HANDLE hSCManager = NULL;

  if ( (':' != *(lpszBinaryPathName+1)) || ('\\' != *(lpszBinaryPathName+2)) )
  {
    return 101;
  }

  #define DRIVE_TYPE_INDETERMINATE 0
  #define ROOT_DIR_DOESNT_EXIST    1

  *lpszRootPathName = *(lpszBinaryPathName+0) ;

  switch (  GetDriveType(lpszRootPathName)  )
  {
    case DRIVE_FIXED :
    { // OK
      break;
    }
    case  ROOT_DIR_DOESNT_EXIST :
    {
      return 101;
    }
    case  DRIVE_TYPE_INDETERMINATE :
    case  DRIVE_REMOVABLE          :
    case  DRIVE_REMOTE             :
    case  DRIVE_CDROM              :
    case  DRIVE_RAMDISK            :
    {
      return 101;
    }
    default :
    {
      return 101;
    }
  }

  if (INVALID_HANDLE_VALUE == CreateFile(lpszBinaryPathName,
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL))
  { 
    return 101;
  }
   
	if((hSCManager = OpenSCManager(
			NULL, 
     		NULL,
     		SC_MANAGER_ALL_ACCESS)) == NULL) {
		return 11;
	}

  schService = CreateService(
        hSCManager,                 // SCManager database
        serviceName,                // name of service
        serviceName,                // name to display (new parameter after october beta)
        SERVICE_ALL_ACCESS,         // desired access
        SERVICE_WIN32_OWN_PROCESS,  // service type
        SERVICE_AUTO_START,         // start type
        SERVICE_ERROR_NORMAL,       // error control type
        lpszBinaryPathName,         // service's binary
        NULL,                       // no load ordering group
        NULL,                       // no tag identifier
        NULL,                       // no dependencies
        NULL,                       // Local System account
        NULL);                      // null password

  if (NULL == schService)
  { switch (GetLastError())
    {
      case ERROR_ACCESS_DENIED :
      { 
        CloseServiceHandle(hSCManager);
        return 102;
        break;
      }
      case ERROR_SERVICE_EXISTS :
      {
        CloseServiceHandle(hSCManager);
        return 103;
        break;
      }
      default :
      {
      }
    }
    CloseServiceHandle(hSCManager);
    return 104;
  }
  else

  CloseServiceHandle(schService);
  CloseServiceHandle(hSCManager);

  return 0;
}

// ---------------------------------------------------------------------------------------


LONG APIENTRY RemoveNTPService(HWND hwnd, LPLONG lpIValue, LPSTR serviceName)
  {
    HANDLE hSCManager = NULL;

	if((hSCManager = OpenSCManager(
			NULL, 
     		NULL,
     		SC_MANAGER_ALL_ACCESS)) == NULL) {
		return 11;
	}

    
  ntpService = OpenService(hSCManager,serviceName,SERVICE_ALL_ACCESS);
  if (NULL == ntpService)
  { switch (GetLastError())
    {
      case ERROR_ACCESS_DENIED :
      { 
             
         CloseServiceHandle(hSCManager);
         return 102;
        break;
      }
      case ERROR_SERVICE_DOES_NOT_EXIST :
      { 
         CloseServiceHandle(hSCManager);
         return 106;
        break;
      }
      default :
       {
           CloseServiceHandle(hSCManager);
           return 105;
      }
    }
  
           CloseServiceHandle(hSCManager);
           return 105;
  }

  if (DeleteService(ntpService))
  {
   CloseServiceHandle(ntpService);
   CloseServiceHandle(hSCManager);
   return 0;
  }
  else
  { switch (GetLastError())
    {
      case ERROR_ACCESS_DENIED :
      { 
        CloseServiceHandle(ntpService);
        CloseServiceHandle(hSCManager);
        return 102;
        break;
      }
      default :
      { 
         CloseServiceHandle(ntpService);
         CloseServiceHandle(hSCManager);
         return 105;
      }
    }
   return 105;
  }
  CloseServiceHandle(ntpService);
  CloseServiceHandle(hSCManager);
 }


LONG APIENTRY addKeysToRegistry(HWND hwnd, LPLONG lpIValue, LPSTR bs)

{
  HKEY hk;                      /* registry key handle */
  BOOL bSuccess;
  UCHAR   myarray[200];
  char *lpmyarray = myarray;
  int arsize = 0;

 /* now add the depends on service key */
 
  /* Create a new key for our application */
  bSuccess = RegCreateKey(HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\NetworkTimeProtocol", &hk);
  if(bSuccess != ERROR_SUCCESS)
    {
      return 1;
    }

  strcpy(lpmyarray,"TcpIp");
  lpmyarray = lpmyarray + 6;
  arsize = arsize + 6;
  strcpy(lpmyarray,"Afd");
  lpmyarray = lpmyarray + 4;
  arsize = arsize + 4;
  strcpy(lpmyarray,"LanManWorkstation");
  lpmyarray = lpmyarray + 18;
  arsize = arsize + 19;
  strcpy(lpmyarray,"\0\0");
  
  bSuccess = RegSetValueEx(hk,  /* subkey handle         */
      "DependOnService",        /* value name            */
      0,                        /* must be zero          */
      REG_MULTI_SZ,             /* value type            */
      (LPBYTE) &myarray,        /* address of value data */
      arsize);                  /* length of value data  */
   if(bSuccess != ERROR_SUCCESS)
    {
      return 1;
    }

  RegCloseKey(hk);
  return 0;
}

LONG APIENTRY StartNTPService(HWND hwnd, LPLONG lpIValue, LPSTR bs)
  {

HANDLE hSCManager = NULL, hNetworkTimeProtocol = NULL;

	if((hSCManager = OpenSCManager(
			NULL, 
     		NULL,
     		SC_MANAGER_ALL_ACCESS)) == NULL) {
		return 11;
	}

  	if((hNetworkTimeProtocol = OpenService(
       		hSCManager,                              
       		TEXT("NetworkTimeProtocol"),               
       		SERVICE_ALL_ACCESS)) == NULL) {
                CloseServiceHandle(hSCManager);
		return 12;
	}

	if(!StartService(hNetworkTimeProtocol, 0, NULL))
                 {
		 if(GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
          { 
          }
		   else
          {
                   CloseServiceHandle(hNetworkTimeProtocol);
                   CloseServiceHandle(hSCManager);
		    return 50;
		  }
		 }
		 
       CloseServiceHandle(hNetworkTimeProtocol);
       CloseServiceHandle(hSCManager);
        return 0;

}
LONG APIENTRY StopNTPService(HWND hwnd, LPLONG lpIValue, LPSTR bs)
  {


DWORD ControlCode = SERVICE_CONTROL_STOP;
HANDLE hSCManager = NULL, hNetworkTimeProtocol = NULL;
SERVICE_STATUS ServiceStatus;

	if((hSCManager = OpenSCManager(
			NULL,
     		NULL,
     		SC_MANAGER_ALL_ACCESS)) == NULL) {
		return 11;
	}

  	if((hNetworkTimeProtocol = OpenService(
       		hSCManager,                              
       		TEXT("NetworkTimeProtocol"),               
       		SERVICE_ALL_ACCESS)) == NULL) {
                CloseServiceHandle(hSCManager);
		return 12;
	}

	if (!ControlService(hNetworkTimeProtocol, ControlCode, &ServiceStatus))
         {
                CloseServiceHandle(hNetworkTimeProtocol);
                CloseServiceHandle(hSCManager);
                //dont return an error here assume it is already stopped.
		return 0;
  	 }
	
  CloseServiceHandle(hNetworkTimeProtocol);
  CloseServiceHandle(hSCManager);
  return 0;

}




LONG APIENTRY LaunchApp(HWND hwnd, LPLONG lpIValue, LPSTR cmd)
{
   BOOLEAN dowait = (BOOLEAN)lpIValue;
   //char arglist[1000];
   STARTUPINFO StartInfo;
   BOOLEAN rvalue;
   PROCESS_INFORMATION phandle;
   DWORD waitval;

    /* Set up members of STARTUPINFO structure. */

    StartInfo.cb = sizeof(STARTUPINFO);
    StartInfo.lpReserved = NULL;
    StartInfo.lpReserved2 = NULL;
    StartInfo.cbReserved2 = 0;
    StartInfo.lpDesktop = NULL;

    // set noshow in case it is a win app
      StartInfo.dwFlags = 0;

	StartInfo.lpTitle = NULL;
    StartInfo.dwX = 1000;
    StartInfo.dwY = 1000;
    StartInfo.dwXSize = 500;
    StartInfo.dwYSize = 500;
    StartInfo.dwXCountChars= 0;
    StartInfo.dwYCountChars = 0;
    StartInfo.dwFillAttribute = 0;
    StartInfo.hStdInput = NULL;
    StartInfo.hStdOutput = NULL;
    StartInfo.hStdError = NULL;

     rvalue = CreateProcess(NULL,cmd,NULL,NULL,FALSE,CREATE_SEPARATE_WOW_VDM,NULL,NULL,&StartInfo,&phandle);
	 if (rvalue == FALSE)
	   {
        return 1;
       }	   
       
 	 if (dowait)
 	   waitval = WaitForSingleObject(phandle.hProcess,INFINITE);
  	 
  	 CloseHandle(phandle.hThread);
	 CloseHandle(phandle.hProcess);
     return 0;
}
