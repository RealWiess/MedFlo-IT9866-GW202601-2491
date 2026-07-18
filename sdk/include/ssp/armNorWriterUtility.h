#ifndef AEMNORWRITERUTILITY_H_75B65487_3E94_44d7_976C_18DFBD0C5FF6__
#define AEMNORWRITERUTILITY_H_75B65487_3E94_44d7_976C_18DFBD0C5FF6__

#include "ite/ith.h"
#include "armNorWriter.h"

#ifdef __cplusplus 
extern "C" { 
#endif

void ArmNorWriterCleanState(void);

/* Set/Get ArmNorWriterStatus */
void SetArmNorWriterStatus(ANW_STATUS writerStatus);
ANW_STATUS GetArmNorWriterStatus(void);

/* Set/Get ArmNorWriterAppStatus */
void SetArmNorWriterAppStatus(ANW_APP_STATUS appStatus);
ANW_APP_STATUS GetArmNorWriterAppStatus(void);

/* Set/Get ArmNorWriterBufLength */
void SetArmNorWriterBufLength(uint32_t romBufLength);
uint32_t GetArmNorWriterBufLength(void);

/* Set/Get ArmNorWriterRomBufAddr */
void SetArmNorWriterRomBufAddr(uint32_t romBufAddr);
uint32_t GetArmNorWriterRomBufAddr(void);

/* Set ArmNorWriterRbBufAddr */
void SetArmNorWriterRbBufAddr(uint32_t rbBufAddr);

/* Set/Get ArmNorWriterAppBar */
void SetArmNorWriterAppProgressMin(uint32_t barMin);
void SetArmNorWriterAppProgressMax(uint32_t barMax);
void SetArmNorWriterAppProgressCurr(uint32_t barCurr);
uint32_t GetArmNorWriterAppProgressMin(void);
uint32_t GetArmNorWriterAppProgressMax(void);
uint32_t GetArmNorWriterAppProgressCurr(void);

#ifdef __cplusplus 
} 
#endif 

#endif //AEMNORWRITERUTILITY_H_75B65487_3E94_44d7_976C_18DFBD0C5FF6__