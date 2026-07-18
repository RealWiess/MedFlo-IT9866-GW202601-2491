@echo off
set PATH=C:\SW code\source code\ITE9868_GWBuild_20260707\tool\bin;C:\ITEGCC_98x\bin;%PATH%
set CFG_PROJECT=GW202601
set CFG_PLATFORM=openrtos
set CFG_BUILDPLATFORM=openrtos
set CMAKE_SOURCE_DIR=C:\SW code\source code\ITE9868_GWBuild_20260707
set PROJECT_ROOT=C:\SW code\source code\ITE9868_GWBuild_20260707
cd /d "%PROJECT_ROOT%\build\openrtos\GW202601"
if exist CMakeCache.txt del CMakeCache.txt
echo Running CMake...
cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="%PROJECT_ROOT%\openrtos\toolchain.cmake" "%PROJECT_ROOT%"
echo Running Make...
make
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo ========================================
echo Build SUCCESS - Post-build steps...
echo ========================================
set BUILD_OUT=%PROJECT_ROOT%\build\openrtos\GW202601\project\GW202601
set ROM_SRC=%BUILD_OUT%\ITE_NOR.ROM
set PKG_SRC=%BUILD_OUT%\ITEPKG03.PKG

:: Generate timestamp for backup
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set TIMESTAMP=%datetime:~0,8%_%datetime:~8,6%

:: Copy ROM with timestamp backup
if exist "%ROM_SRC%" (
    copy /Y "%ROM_SRC%" "%PROJECT_ROOT%\ITE_NOR.ROM"
    copy /Y "%ROM_SRC%" "%PROJECT_ROOT%\GW202601_%TIMESTAMP%.ROM"
    echo [OK] ROM copied: ITE_NOR.ROM + GW202601_%TIMESTAMP%.ROM
) else (
    echo [WARN] ROM not found at %ROM_SRC%
)

:: Copy PKG
if exist "%PKG_SRC%" (
    copy /Y "%PKG_SRC%" "%PROJECT_ROOT%\ITEPKG03.PKG"
    echo [OK] PKG copied: ITEPKG03.PKG
) else (
    echo [WARN] PKG not found at %PKG_SRC%
)

echo ========================================
echo Post-build complete. Ready for OTA.
echo ========================================
pause
