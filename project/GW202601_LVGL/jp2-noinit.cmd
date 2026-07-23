@echo off

pushd ..\..\..
call common.cmd
popd

rem glamomem -t glamomem.dat -i -q
rem glamomem -t glamomem.dat -l GW202601_LVGL.bin

if "9860" == "970" (
    glamomem -t glamomem.dat -q -e C:/SW code/source code/ITE9868_GWBuild_2491_20260718/sdk/target/debug/win32/fa626/JTAG_Switch_32bits.txt
) else (
    if "9860" == "9860" (
        glamomem -t glamomem.dat -q -e C:/SW code/source code/ITE9868_GWBuild_2491_20260718/sdk/target/debug/win32/fa626/JTAG_Switch_32bits.txt
    ) else (
        glamomem -t glamomem.dat -q -e C:/SW code/source code/ITE9868_GWBuild_2491_20260718/sdk/target/debug/win32/fa626/JTAG_Switch_16bits.txt
    )
)

openocd -f C:/SW code/source code/ITE9868_GWBuild_2491_20260718/sdk/target/debug/win32/fa626/fa626.cfg
