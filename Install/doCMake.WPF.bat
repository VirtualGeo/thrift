@echo off

call "%~dp0setEnvironment.WPF.bat"

if not exist %THRIFT_GENERATED% (
	mkdir %THRIFT_GENERATED%
)

if not exist %THRIFT_RELEASE% (
	mkdir %THRIFT_RELEASE%
)

set BISON_DIR=%THRIFT_EXTERNAL%\win_flex_bison-latest

rem ***************************************************************************
rem Find proper generator
rem ***************************************************************************

call "%~dp0configureCompiler.bat"
if ERRORLEVEL 1 goto :error

pushd %THRIFT_GENERATED%

cmake %THRIFT_ROOT% ^
	-G%GENERATOR% ^
	-DWITH_MT=OFF ^
	-DWITH_SHARED_LIB=OFF ^
	-DWITH_STATIC_LIB=ON ^
	-DBISON_EXECUTABLE=%BISON_DIR%\win_bison.exe ^
	-DFLEX_EXECUTABLE=%BISON_DIR%\win_flex.exe ^
	-DCMAKE_INSTALL_PREFIX=%THRIFT_RELEASE% ^
	-DLIBEVENT_ROOT=%VRGIS_EXTERNAL%\libevent-2.1.8 ^
	-DLibEvent_LIBRARIES_PATHS=%VRGIS_EXTERNAL%\libevent-2.1.8\lib\%ZLIB_SUFFIX% ^
	-DOPENSSL_ROOT_DIR=%VR_EXTERNAL%\openssl-%VR_OPENSSL_VERSION% ^
	-DOPENSSL_USE_STATIC_LIBS=OFF ^
	-DZLIB_LIBRARY=%VR_EXTERNAL%\zlib-%VR_ZLIB_VERSION%\lib\%ZLIB_SUFFIX%\zlibstatic.lib ^
	-DZLIB_ROOT=%VR_EXTERNAL%\zlib-%VR_ZLIB_VERSION% ^
	-DBOOST_ROOT=%VR_EXTERNAL%\boost%VR_BOOST_VERSION% ^
	-DBOOST_LIBRARYDIR=%VR_EXTERNAL%\boost%VR_BOOST_VERSION%\stage\lib ^
	-DWITH_BOOST_STATIC=ON ^
	-DSSL_EAY_RELEASE=%VR_EXTERNAL%\openssl-%VR_OPENSSL_VERSION%\lib\%ZLIB_SUFFIX%\ssleay32.lib ^
	-DLIB_EAY_RELEASE=%VR_EXTERNAL%\openssl-%VR_OPENSSL_VERSION%\lib\%ZLIB_SUFFIX%\libeay32.lib ^
	-DBUILD_TESTING=OFF ^
	-DBUILD_EXAMPLES=OFF ^
	-DWITH_JAVA=ON ^
	-DBUILD_PYTHON=OFF ^
	-DTHRIFT_COMPILER_C_GLIB=OFF

popd

pause
exit /b 0

:error
exit /b 42
