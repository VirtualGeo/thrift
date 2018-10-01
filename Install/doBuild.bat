@echo off
call "%~dp0setEnvironment.bat"

cmake --build %THRIFT_GENERATED% -- /nologo /v:q
cmake --build %THRIFT_GENERATED% --target INSTALL -- /nologo /v:q
REM cmake --build %THRIFT_GENERATED% --config Build -- /nologo /v:q

pause
