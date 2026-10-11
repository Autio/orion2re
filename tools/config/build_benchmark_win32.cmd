@echo off
setlocal
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do set "VS=%%i"
call "%VS%\VC\Auxiliary\Build\vcvars32.bat"
if errorlevel 1 exit /b 1
set "PATH=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
cmake -S "%~dp0..\.." -B "%TEMP%\orion2re-incremental-win32" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DORION2RE_WINDOWS_ARCH=x86 -DORION2RE_ENABLE_ASAN=ON -DORION2RE_BUILD_CONFIG_TOOL=OFF -DORION2RE_GAME_DIR= "-DCMAKE_CXX_FLAGS_DEBUG=/Zi /Ob0 /Od" "-DCMAKE_C_FLAGS_DEBUG=/Zi /Ob0 /Od"
if errorlevel 1 exit /b 1
cmake --build "%TEMP%\orion2re-incremental-win32" --target config-load-benchmark --parallel 4
if errorlevel 1 exit /b 1
if not "%~1"=="" "%TEMP%\orion2re-incremental-win32\config-load-benchmark.exe" "%~1" "%~2" %3
exit /b %errorlevel%