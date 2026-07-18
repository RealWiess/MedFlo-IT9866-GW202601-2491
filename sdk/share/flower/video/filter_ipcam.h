#ifndef FILTER_IPCAM_H
#define FILTER_IPCAM_H

#ifdef __cplusplus
extern "C" {
#endif

int PutIntoPacketQueue(unsigned char* inputbuf, int inputbuf_size, double timestamp);

#ifdef __cplusplus
}
#endif

#endif

