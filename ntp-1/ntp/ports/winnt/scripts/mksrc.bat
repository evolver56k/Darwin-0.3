REM zip -r winnt-ntp4-src.zip *.* -x *.obj *.pch *.bsc *.pdb *.sbr nt*.zip *.tar *.gz *.ilk beta*.zip
REM
tar cvfX ..\winnt-ntp-src.tar excludes *
gzip ..\winnt-ntp-src.tar 
