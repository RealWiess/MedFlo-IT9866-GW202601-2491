@echo off

pushd ..\..\..
call common.cmd
popd

set PATH=D:\parasoft\cpptest\bin;%PATH%
set CFG_PROJECT=GW202601_LVGL_v2
set CFG_PLATFORM=openrtos
D:\parasoft\cpptest\cpptest.exe
pause
