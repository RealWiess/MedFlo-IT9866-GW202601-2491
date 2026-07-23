@echo off

pushd ..\..\..
call common.cmd
popd

C:/SW code/source code/ITE9868_GWBuild_2491_20260718/tool/bin/libCodeSize C:/ITEGCC/BIN/arm-none-eabi-readelf.exe C:/SW code/source code/ITE9868_GWBuild_2491_20260718/lib/fa626 C:/SW code/source code/ITE9868_GWBuild_2491_20260718/project/bootloader/bootloader.symbol bootloader_CodeSize.txt

pause
