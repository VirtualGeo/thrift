@echo off
call "%~dp0setEnvironment.WPF.bat"

set BUILD_CMD=cmake --build %THRIFT_GENERATED%
set VISUAL_POSTFIX=-- /nologo /v:q

echo Build debug...
%BUILD_CMD% --config Debug %VISUAL_POSTFIX%
echo Done. Installing...
%BUILD_CMD%  --config Debug --target INSTALL %VISUAL_POSTFIX%
if ERRORLEVEL 1 goto :error

echo Build release...
%BUILD_CMD% --config Release %VISUAL_POSTFIX%
echo Done. Installing...
%BUILD_CMD% --config Release --target INSTALL %VISUAL_POSTFIX%
if ERRORLEVEL 1 goto :error

call "%~dp0configureCompiler.bat"
if ERRORLEVEL 1 goto :error

rem Build C# Libraries
pushd %~dp0..\lib\csharp\src

msbuild Thrift.sln /p:Configuration=%CS_CONFIGURATION%Release /p:Platform=%CS_PLATFORM% /t:Clean,Build
if ERRORLEVEL 1 (
	popd
	goto :error
)

msbuild Thrift.sln /p:Configuration=%CS_CONFIGURATION%Debug /p:Platform=%CS_PLATFORM% /t:Clean,Build
if ERRORLEVEL 1 (
	popd
	goto :error
)

rem We will need to build java libraries aswell

pause
exit /b 0

:error
pause
exit /b 42

