@echo off

cd C:/SW code/source code/ITE9868_GWBuild_2491_20260718\build\openrtos\GW202601\project\GW202601

glamomem -t glamomem.dat -i -q
glamomem -t glamomem.dat -l GW202601.bin

if "9860" == "970" (
    glamomem -t glamomem.dat -R 0x00000001 -a 0xd8300000
) else (
    if "9860" == "9860" (
        glamomem -t glamomem.dat -R 0x00000001 -a 0xd8300000
    ) else (
        glamomem -t glamomem.dat -R 0x0025 -a 0x1F00
    )
)
