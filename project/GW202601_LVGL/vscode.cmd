@echo off

pushd ..\..\..
call common.cmd
popd

set CFG_PROJECT=GW202601_LVGL
set CFG_PLATFORM=openrtos
set CFG_BUILDPLATFORM=openrtos
code -n C:/SW code/source code/ITE9868_GWBuild_2491_20260718
