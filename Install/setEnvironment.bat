@echo off

rem **************************************************************************
rem Look for an overrideEnvironment.bat in this batch file folder
rem It is ignored by SVN and you can set your VR_SETENVIRONMENT var there
rem **************************************************************************
if not exist "%~dp0\overrideEnvironment.bat" goto :noOverrideEnvironment
call "%~dp0\overrideEnvironment.bat"
:noOverrideEnvironment

if not defined THRIFT_ROOT (
    set THRIFT_ROOT=%~dp0..
)

rem ***************************************************************************
rem Since we need both Vertigo and VrGIS externals, call VrGIS
rem setEnvironment directly
rem ***************************************************************************
if not defined VRGIS_SETENVIRONMENT (
    set VRGIS_SETENVIRONMENT=%~dp0..\..\VrGIS\Install
)
call "%VRGIS_SETENVIRONMENT%\setEnvironment.bat" %*
if ERRORLEVEL 1 goto :error

if not defined THRIFT_GENERATED_BASE (
	set THRIFT_GENERATED_BASE=%THRIFT_ROOT%
)

if not defined THRIFT_EXTERNAL_BASE (
	set THRIFT_EXTERNAL_BASE=%THRIFT_ROOT%
)

rem ***************************************************************************
rem Setup thrift external folder
rem (for externals that neither vertigo or VrGIS are using)
rem ***************************************************************************

if not defined THRIFT_EXTERNAL (
	set THRIFT_EXTERNAL=%THRIFT_EXTERNAL_BASE%\External_Win
)

if not defined THRIFT_GENERATED (
	set THRIFT_GENERATED=%THRIFT_GENERATED_BASE%\%GENERATED_DIR%
)

rem **************************************************************************
rem Lists Thrift defined environment variables
rem **************************************************************************
SET THRIFT_

goto :end

:error
echo Error running VrGIS setEnvironment.bat
exit /b 42

:end
exit /b 0
