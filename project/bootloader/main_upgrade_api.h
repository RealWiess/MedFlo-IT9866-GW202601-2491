#ifndef MAIN_UPGRADE_API_H
#define MAIN_UPGRADE_API_H

#ifdef __cplusplus
extern "C"
{
#endif

void BootloaderShowLogo();
void BeginLoadImage(void);
void EndLoadImage(bool doBoot);
void InitFileSystem(void);
void InitLcdConsole(void);
void DoUpgrade(void);
bool DetectUsbDeviceMode(void);
void DoBootTestBin(void);

#ifdef __cplusplus
}
#endif

#endif // BOOTLOADER_H
