
#ifndef ITE_USBD_ECM_H
#define ITE_USBD_ECM_H

#ifdef __cplusplus
extern "C" {
#endif


int iteUsbdEcmRegister(void);
int iteUsbdEcmUnRegister(void);

int iteUsbdEcmOpen(uint8_t* mac_addr, void(*func)(void *arg, void *packet, int len), void* arg);
int iteUsbdEcmStop(void);
int iteUsbdEcmSend(void* packet, uint32_t len);
int iteUsbdEcmGetLink(void);
int iteUsbdEcmSetRxMode(int flags);

#ifdef __cplusplus
}
#endif

#endif // ITE_USBD_NCM_H
