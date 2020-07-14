
#include <windows.h>

int main(int argc, char* argv[])
{
    char 	fpath[MAX_PATH],full[MAX_PATH];
    LPTSTR	end;
    DWORD	volattr;

    if (argc<2)
       exit(256);
    strcpy(fpath,argv[1]);
    end=fpath;
    while(((end=strchr(fpath,'/'))))
        *end='\\';
    GetFullPathName(fpath,
                    MAX_PATH,
                    full,
                    0);
    strcat(full,"\\*");
    end=strstr(full,":");
    if (!end) {
       end=strstr(full,"\\");
       if (end)
          end++;
    }
    if (end) {
       end++;
       *end=0;
    }
    if (GetVolumeInformation(full,
                             0,
                             0,
                             0,
                             0,
                             &volattr,
                             0,
                             0))
      {
       printf("case sensitive file name lookup: %s\n",
              (volattr & FS_CASE_SENSITIVE) ? "supported" : "NOT supported");
       printf("case preserved on disk: %s\n",
              (volattr & FS_CASE_IS_PRESERVED) ? "yes" : "NO");
      }
}