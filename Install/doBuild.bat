@echo off
call "%~dp0setEnvironment.bat"

set BUILD_CMD=cmake --build %THRIFT_GENERATED%
set VISUAL_POSTFIX=-- /nologo /v:q

echo Build debug...
%BUILD_CMD% --config Debug %VISUAL_POSTFIX%
echo Done. Installing...
%BUILD_CMD%  --config Debug --target INSTALL %VISUAL_POSTFIX%

echo Build release...
%BUILD_CMD% --config Release %VISUAL_POSTFIX%
echo Done. Installing...
%BUILD_CMD% --config Release --target INSTALL %VISUAL_POSTFIX%
REM %BUILD_CMD% --config Build %VISUAL_POSTFIX%

pause
