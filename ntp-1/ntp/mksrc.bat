REM zip -r ntxntp35f-src.zip *.* -x *.obj *.pch *.bsc *.pdb *.sbr nt*.zip *.tar *.gz *.ilk beta*.zip
REM
tar cvfX ..\ntxntp-src.tar excludes *
gzip ..\ntxntp-src.tar 
