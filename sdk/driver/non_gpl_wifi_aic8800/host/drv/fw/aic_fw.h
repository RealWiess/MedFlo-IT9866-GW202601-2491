#ifndef _AIC_FW_H_
#define _AIC_FW_H_

enum aic_fw {
    FW_UNKNOWN = 0,
    FMACFW,
    FMACFW_RF,
    FW_ADID,
    FW_PATCH,
    FW_PATCH_TABLE,
    FW_ADID_U03,
    FW_PATCH_U03,
    FW_PATCH_TABLE_U03,
    FMACFW_8800DC,
    FMACFW_RF_8800DC,
    FMACFW_PATCH_8800DC,
    FMACFW_RF_PATCH_8800DC,
    LMACFW_RF_8800DC,
    FMACFW_PATCH_8800DC_U02,
    FMACFW_PATCH_TBL_8800DC_U02,
    FW_ADID_8800DC,
    FW_PATCH_8800DC,
    FW_PATCH_TABLE_8800DC,
    FW_ADID_8800DC_U02,
    FW_PATCH_8800DC_U02,
    FW_PATCH_TABLE_8800DC_U02,
    FW_ADID_8800D80,
    FW_ADID_8800D80_U02,
    FMACFW_8800D80,
    FMACFW_8800D80_U02,
    FMACFW_RF_8800D80,
    FMACFW_RF_8800D80_U02,
    FW_PATCH_8800D80,
    FW_PATCH_8800D80_U02,
    FW_PATCH_TABLE_8800D80,
    FW_PATCH_TABLE_8800D80_U02,
};

void *aic8800d_fw_ptr_get(void);
uint32_t aic8800d_fw_size_get(void);
void *aic8800d_rf_fw_ptr_get(void);
uint32_t aic8800d_rf_fw_size_get(void);

void *aic8800d80_fw_ptr_get(void);
uint32_t aic8800d80_fw_size_get(void);
void *aic8800d80_rf_fw_ptr_get(void);
uint32_t aic8800d80_rf_fw_size_get(void);
void *aic8800d80_u02_fw_ptr_get(void);
uint32_t aic8800d80_u02_fw_size_get(void);
void *aic8800d80_u02_rf_fw_ptr_get(void);
uint32_t aic8800d80_u02_rf_fw_size_get(void);

#endif
