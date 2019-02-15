@echo off

call "%~dp0setEnvironment.bat"

if not exist %THRIFT_GENERATED% (
	mkdir %THRIFT_GENERATED%
)

set BISON_DIR=%THRIFT_EXTERNAL%\win_flex_bison-latest

rem ***************************************************************************
rem Find proper generator
rem ***************************************************************************

call "%~dp0configureCompiler.bat"
if ERRORLEVEL 1 goto :error

pushd %THRIFT_GENERATED%

REM conan export %THRIFT_ROOT% dxt/stable
conan install %THRIFT_ROOT% ^
	-s compiler.version=%COMPILER_VERSION% ^
	-s arch=%ARCH_TYPE% ^
	-s build_type=Debug ^
	-o thrift:with_java=True

REM conan build --configure %THRIFT_ROOT%

popd

pause
exit /b 0

:error
exit /b 42
