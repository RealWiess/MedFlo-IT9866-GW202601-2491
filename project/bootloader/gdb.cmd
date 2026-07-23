@echo off

pushd ..\..\..
call common.cmd
popd

arm-none-eabi-gdb -x C:/SW code/source code/ITE9868_GWBuild_2491_20260718/sdk/target/debug/win32/fa626/init-gdb bootloader
if errorlevel 1 pause
