@echo off
call "%~dp0setEnvironment.bat"

cmake --build %THRIFT_GENERATED%
REM cmake --build %THRIFT_GENERATED% --config Build -- /nologo /v:q

pause
