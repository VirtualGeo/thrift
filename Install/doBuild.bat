@echo off
call "%~dp0setEnvironment.bat"

set BUILD_CMD=cmake --build %THRIFT_GENERATED%
set VISUAL_POSTFIX=-- /nologo /v:q

pushd %THRIFT_GENERATED%
conan build %THRIFT_ROOT%
REM conan export %THRIFT_ROOT% thrift/0.12.1@dxt/stable
popd

exit /b 0
