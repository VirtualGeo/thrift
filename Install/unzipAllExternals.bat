@echo off

call "%~dp0setEnvironment.bat"

if not exist %THRIFT_EXTERNAL% (
	mkdir %THRIFT_EXTERNAL%
)

set BISON_ZIP_URL=https://sourceforge.net/projects/winflexbison/files/win_flex_bison-latest.zip/download
set BISON_ZIP=%THRIFT_EXTERNAL%\win_flex_bison-latest.zip
bitsadmin /transfer BisonDL /download /priority normal %BISON_ZIP_URL% %BISON_ZIP%

7z.exe x %BISON_ZIP% -o%THRIFT_EXTERNAL%\win_flex_bison-latest -aoa

pause
