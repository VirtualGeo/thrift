@echo off

if %CURRENT_COMPILER% == Win32VC12 (
	set GENERATOR="Visual Studio 12 2013"
	set ZLIB_SUFFIX=vc12
	goto :done
)

if %CURRENT_COMPILER% == Win64VC12 (
	set GENERATOR="Visual Studio 12 2013 Win64"
	set ZLIB_SUFFIX=vc12.x64
	goto :done
)

if %CURRENT_COMPILER% == Win32VC14 (
	set GENERATOR="Visual Studio 14 2015"
	set ZLIB_SUFFIX=vc14
	goto :done
)

if %CURRENT_COMPILER% == Win64VC14 (
	set GENERATOR="Visual Studio 14 2015 Win64"
	set ZLIB_SUFFIX=vc14.x64
	goto :done
)

if %CURRENT_COMPILER% == Win32VC15 (
	set GENERATOR="Visual Studio 15 2017"
	set ZLIB_SUFFIX=vc14
	goto :done
)

if %CURRENT_COMPILER% == Win64VC15 (
	set GENERATOR="Visual Studio 15 2017 Win64"
	set ZLIB_SUFFIX=vc14.x64
	goto :done
)

echo Compiler %CURRENT_COMPILER% is unknown or not supported.
exit /B 1

:done
exit /B
