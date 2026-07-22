@echo off
setlocal enabledelayedexpansion
set ROOT=%~dp0
set ROOT=%ROOT:~0,-1%
set PATH=%ROOT%\tool\bin;C:\ITEGCC_98x\bin;%PATH%
set CFG_PROJECT=gw_lvgl_test
set CFG_PLATFORM=openrtos
set CFG_BUILDPLATFORM=openrtos
set CMAKE_SOURCE_DIR=%ROOT%
set PROJECT_ROOT=%ROOT%

echo ========================================
echo   gw_lvgl_test Build (Pure SDK lv_demos)
echo ========================================
echo.

if not exist "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%" mkdir "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%"
cd /d "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%"
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rd /s /q CMakeFiles

:: Create required stub dirs
if not exist "lib\sm32" mkdir "lib\sm32"
if not exist "lib\sm32\mp3.codecs" type nul > "lib\sm32\mp3.codecs"
if not exist "lib\sm32\wave.codecs" type nul > "lib\sm32\wave.codecs"
if not exist "data\private" mkdir "data\private"
if not exist "data\public" mkdir "data\public"
if not exist "data\temp" mkdir "data\temp"
if not exist "lib\fa626" mkdir "lib\fa626"
if not exist "lib\fa626\tlb_wt.o" copy "%PROJECT_ROOT%\..\ITE9868_GWBuild_2470_20260707UI\build\openrtos\GW202601\lib\fa626\tlb_wt.o" "lib\fa626\tlb_wt.o" >nul 2>&1

echo [1/4] CMake...
cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="%PROJECT_ROOT%\openrtos\toolchain.cmake" -B "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%" -S "%PROJECT_ROOT%"
if %ERRORLEVEL% NEQ 0 (echo [ERROR] CMake failed! & pause & exit /b %ERRORLEVEL%)
echo [OK]

echo [2/4] Make...
make -j8 -C "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%"
echo.

echo [3/4] Patch pkgtool...
set BUILD=%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%
if exist "%BUILD%\project\bootloader\CMakeFiles\bootloader.dir\build.make" (
    powershell -Command "(Get-Content '%BUILD%\project\bootloader\CMakeFiles\bootloader.dir\build.make' -Raw) -replace '--unformatted-device 1 --unformatted-index 0 --unformatted-data','--nor-unformatted-data0' -replace '--unformatted-device 1 --unformatted-index 1 --unformatted-data','--nor-unformatted-data1' -replace '--unformatted-device 1 --unformatted-index 2 --unformatted-data','--nor-unformatted-data2' -replace '--unformatted-device 1 --unformatted-size','--nor-unformatted-size' -replace '--unformatted-pos','--nor-unformatted-data1-pos' -replace '--partition-device 1 --partition-index 0 --partition-dir','--nor-partiton0-dir' -replace '--partition-device 1 --partition-index 1 --partition-dir','--nor-partiton1-dir' -replace '--partition-device 1 --partition-index 2 --partition-dir','--nor-partiton2-dir' -replace '--partition-device 1 --partition-index 3 --partition-dir','--nor-partiton3-dir' -replace '--partition-device 1 --partition-index 0 --partition-size','--nor-partiton0-size' -replace '--partition-device 1 --partition-index 1 --partition-size','--nor-partiton1-size' -replace '--partition-device 1 --partition-index 2 --partition-size','--nor-partiton2-size' -replace '--partition-device 1 --partition-index 3 --partition-size','--nor-partiton3-size' -replace ' --unformatted-device 1\b','' -replace ' --partition-device 1\b','' -replace ' --partition\b','' | Set-Content '%BUILD%\project\bootloader\CMakeFiles\bootloader.dir\build.make'"
)
if exist "%BUILD%\project\%CFG_PROJECT%\CMakeFiles\%CFG_PROJECT%.dir\build.make" (
    powershell -Command "(Get-Content '%BUILD%\project\%CFG_PROJECT%\CMakeFiles\%CFG_PROJECT%.dir\build.make' -Raw) -replace '--unformatted-device 1 --unformatted-index 0 --unformatted-data','--nor-unformatted-data0' -replace '--unformatted-device 1 --unformatted-index 1 --unformatted-data','--nor-unformatted-data1' -replace '--unformatted-device 1 --unformatted-index 2 --unformatted-data','--nor-unformatted-data2' -replace '--unformatted-device 1 --unformatted-size','--nor-unformatted-size' -replace '--unformatted-pos','--nor-unformatted-data1-pos' -replace '--partition-device 1 --partition-index 0 --partition-dir','--nor-partiton0-dir' -replace '--partition-device 1 --partition-index 1 --partition-dir','--nor-partiton1-dir' -replace '--partition-device 1 --partition-index 2 --partition-dir','--nor-partiton2-dir' -replace '--partition-device 1 --partition-index 3 --partition-dir','--nor-partiton3-dir' -replace '--partition-device 1 --partition-index 0 --partition-size','--nor-partiton0-size' -replace '--partition-device 1 --partition-index 1 --partition-size','--nor-partiton1-size' -replace '--partition-device 1 --partition-index 2 --partition-size','--nor-partiton2-size' -replace '--partition-device 1 --partition-index 3 --partition-size','--nor-partiton3-size' -replace ' --unformatted-device 1\b','' -replace ' --partition-device 1\b','' -replace ' --partition\b','' | Set-Content '%BUILD%\project\%CFG_PROJECT%\CMakeFiles\%CFG_PROJECT%.dir\build.make'"
)
echo [OK]

echo [4/4] Make (fixed pkgtool)...
make -j8 -C "%PROJECT_ROOT%\build\openrtos\%CFG_PROJECT%"

set BUILD_OUT=%ROOT%\build\openrtos\%CFG_PROJECT%\project\%CFG_PROJECT%
if exist "%BUILD_OUT%\ITEPKG03.PKG" (
    cd /d "%BUILD_OUT%"
    pkgtool -s 0x1000000 -o ITE_NOR.ROM -n ITEPKG03.PKG
    for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
    set TIMESTAMP=!datetime:~0,8!_!datetime:~8,6!
    copy /Y ITE_NOR.ROM "%ROOT%\ITE_NOR.ROM"
    copy /Y ITE_NOR.ROM "%ROOT%\gw_lvgl_test_!TIMESTAMP!.ROM"
    copy /Y ITEPKG03.PKG "%ROOT%\ITEPKG03.PKG"
    echo [OK] gw_lvgl_test_!TIMESTAMP!.ROM
)
echo ========================================
echo Done.
echo ========================================
pause
