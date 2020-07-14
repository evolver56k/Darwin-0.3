/*
 * File: uninst.cpp
 *
 * Purpose: To Uninstall a Windows NT Service during an InstallShield uninstall
 *
 */

#include <windows.h>
#include "tchar.h"
#include "resource.h"


BOOL GetBaseDir(LPTSTR,char*);
void SplitName(const char*,char*,char*,char);
BOOL RemoveService();
char szBaseDir[_MAX_PATH];
HANDLE hInstDLL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInstDLL = hinstDLL;
   	return (1);
}

LONG APIENTRY UninstInitialize(HWND hwndDlg,HANDLE hInstance,LONG lRes)
{ LPTSTR lpStr;
	char szError[1024];
	//char szTemp[1024];

	//Figure where the LOG and DLL file are located
    lpStr=GetCommandLine();

	GetBaseDir(lpStr,szBaseDir);
	
	//wsprintf(szTemp,"Executing UninstInitialize\n\n Log File Location:%s",szBaseDir);

	//MessageBox(GetFocus(), szTemp ,"",MB_OK);

	if (RemoveService() == FALSE)
	{
		LoadString (hInstDLL, ERROR_ABORT, szError, sizeof(szError));
		MessageBox(NULL, szError, " ", MB_OK);
		return -1;
	}

	return(0);
}


LONG APIENTRY UninstallService (LPCTSTR szPath)
{ 
	char szError[1024];

    strcpy (szBaseDir, szPath);
	if (RemoveService() == FALSE)
	{
		LoadString (hInstDLL, ERROR_ABORT, szError, sizeof(szError));
		MessageBox(NULL, szError, " ", MB_OK);
		return -1;
	}

	return(0);
}

LONG APIENTRY UninstUnInitialize(HWND hwndDlg,HANDLE hInstance,LONG lRes)
{
	//MessageBox(GetFocus(),"Executing UninstUnInitialize","",MB_OK);

	return (0);
}

// Check the last character in a string to see if it is a given character
BOOL LastChar(const char* szString,char chSearch)
{ LPTSTR lpstr,lplast;

	if (lstrlen(szString)==0)
		return(FALSE);

	lplast=(LPTSTR)szString;
	lpstr=CharNext(szString);
	while (*lpstr)
	{
		lplast=lpstr;
		lpstr=CharNext(lplast);
	} 

	if (IsDBCSLeadByte(*lplast))
		return(FALSE);

	if (*lplast==chSearch)
		return(TRUE);
	else
		return(FALSE);
}

//Get the directory where the DLL is found
BOOL GetBaseDir(LPTSTR lpLine,char* szBase)
{ char szLine[_MAX_PATH],szFile[_MAX_PATH];
  char* ptr;
  int len;

	lstrcpy(szLine,lpLine);
	ptr=_tcsstr(szLine,"-c");
	if (ptr==NULL)
	{
		szBase[0]=0;
		return(0);
	}

	ptr=CharNext(ptr); //skip past -c and up to filename portion
	ptr=CharNext(ptr); 
	//remove leading " if present
	if ((IsDBCSLeadByte(*ptr)==0) && (*ptr=='"'))
		++ptr;
	lstrcpy(szFile,ptr);
	//remove trailing " if present
	if (LastChar(szFile,'"'))
	{
		len=lstrlen(szFile);
		szFile[len-1]=0;
	}
	//split the path\my.dll string
	SplitName(szFile,szBase,szLine,'\\');
	return(TRUE);
}

//Split a string at the last occurance of a given character
void SplitName(const char* szWholeName,char* szPath,char* szFile,char chSep)
{ int i,len,j;
  LPTSTR lpstr,lplast;

	len=i=lstrlen(szWholeName);
	if (len==0)
	{
		szPath[0]=0; szFile[0]=0; return;
	}

	//First walk out to the end of the string
	lplast=(LPTSTR)szWholeName;
	lpstr=CharNext(szWholeName);
	while (*lpstr)
	{
		lplast=lpstr;
		lpstr=CharNext(lplast);
	} 

	//Now walk backward to the beginning
	while (*lplast)
	{
		if (IsDBCSLeadByte(*lplast))
		{
			lplast=CharPrev(szWholeName,lplast);
			if (lplast==szWholeName)
			{
				szPath[0]=0;
				lstrcpy(szFile,szWholeName);
				return;
			}
		}
		else if (*lplast==chSep)
		{										 
			i=lplast-szWholeName+1;
			lstrcpyn(szPath,szWholeName,i+1);
			szPath[i]=0;
			j=len-i;
			lstrcpyn(szFile,&szWholeName[i],j+1);
			szFile[j]=0;
			return;
		}
		else
		{
			lplast=CharPrev(szWholeName,lplast);
			if (lplast==szWholeName)
			{
				szPath[0]=0;
				lstrcpy(szFile,szWholeName);
				return;
			}
		}
	}

	//Didn't find it so just default the values
	szPath[0]=0;
	lstrcpy(szFile,szWholeName);
}




//
//  FUNCTION: RemoveService()
//
//  PURPOSE: Stops and removes the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    TRUE or FALSE
//
//  COMMENTS: Checks the service.ini file in the same directory as
//  uninst.dll for the service name to remove.
//
BOOL RemoveService()
{
    SC_HANDLE   schService;
    SC_HANDLE   schSCManager;
	SERVICE_STATUS          ssStatus;       // current status of the service


    char   szSectionName[9]				= "Services";
    char   szDefault[1]					= "";
	char   szServiceName[512]			= {0};
	char   szServiceNameList[1024]		= {0};
	char   szKeyName[256]				= {0};
	char   szError[1024]				= {0};
	int	   i, j;
	char   szTemp[1024]					= {0};
	DWORD  nRes;



	wsprintf(szTemp,"%sservice.ini",szBaseDir);

	if ( GetPrivateProfileString ((LPCTSTR) szSectionName, NULL,  (LPCTSTR) szDefault, szServiceNameList,
		sizeof (szServiceNameList), szTemp) == 0)
	{
		nRes = GetLastError();
		LoadString (hInstDLL, ERROR_LOAD_INI, szError, sizeof(szError));
		wsprintf(szError,"%d %s\n\n%s", nRes, szError, szTemp);
		MessageBox(NULL, szError, " ", MB_OK);
		return FALSE;
	}


	i = 0;
	j = 0;
	while (!((szServiceNameList[i -1] == 0) && (szServiceNameList[i] == 0)))
	{

		szKeyName[j] = szServiceNameList[i];
		if (szServiceNameList[i] == 0) //found key name
		{
			j = -1;
			//use key name to get service name
			if (GetPrivateProfileString ((LPCTSTR) szSectionName, szKeyName, (LPCTSTR) szDefault,  szServiceName,
				sizeof(szServiceName), szTemp) == 0)
			{
				nRes = GetLastError();
				LoadString (hInstDLL, ERROR_LOAD_INI, szError, sizeof(szError));
				wsprintf(szError,"%d %s\n\n%s\n\n%s", nRes, szError, szTemp, szKeyName);
				MessageBox(NULL, szError, " ", MB_OK);
				return FALSE;
			}

			schSCManager = OpenSCManager(
								NULL,                   // machine (NULL == local)
								NULL,                   // database (NULL == default)
								SC_MANAGER_ALL_ACCESS   // access required
								);

			if ( !schSCManager ) 
			{
				LoadString (hInstDLL, ERROR_OPENSCMANAGER_FAIL, szError, sizeof(szError));
				MessageBox(NULL, szError, " ", MB_OK);
				return FALSE;
			}

			schService = OpenService(schSCManager, szServiceName, SERVICE_ALL_ACCESS);

			if (!schService) 
			{
				LoadString (hInstDLL, ERROR_OPENSERVICE_FAIL, szError, sizeof(szError));
				wsprintf(szError,"%s\n\n%s",szError, szServiceName);
				MessageBox(NULL, szError, " ", MB_OK);
				return FALSE;
			}

			// try to stop the service
			if ( ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus ) )
			{
				Sleep( 1000 );

				while( QueryServiceStatus( schService, &ssStatus ) )
				{
					if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
					{
						Sleep( 1000 );
					}
					else
						break;
				}

				if ( ssStatus.dwCurrentState != SERVICE_STOPPED )
				{
					LoadString (hInstDLL, ERROR_SERVICE_NOT_STOPPED, szError, sizeof(szError));
					wsprintf(szError,"%s\n\n%s",szError, szServiceName);
					MessageBox(NULL, szError, " ", MB_OK);
					return FALSE;
				}

			}

			// now remove the service
			if( !(DeleteService(schService)) )
				{
					LoadString (hInstDLL, ERROR_SERVICE_NOT_DELETED, szError, sizeof(szError));
					wsprintf(szError,"%s\n\n%s",szError, szServiceName);
					MessageBox(NULL, szError, " ", MB_OK);
					return FALSE;
				}

			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);

		}
		i++;
		j++;
	}
	return TRUE;
}

