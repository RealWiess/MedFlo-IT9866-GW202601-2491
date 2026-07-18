#include <stdio.h>
#include <string.h>
#include "usbhcc/api/api_mdriver.h"
#include "usbhcc/api/api_scsi_tgt.h"

#define ITP_MST_MAX_LUNS       4

//<<<<
/** from config_scsi_tgt.h */
uint8_t N_LUNS_MST = 0;

/** from config_scsi_tgt.c */
scsim_table_entry_t  scsi_media_table_mst[ITP_MST_MAX_LUNS] = { 0 };

/** from scsi_tgt.c and config_scsi_tgt.h */

/* SCSI identification information. Sent reported in the response of
the inquiry command. The length of these strings must match the
one specified in the comment. Use space characters to padd to required
size when needed. */

/* 8 byte long vendor id. */
#define SCSI0_VENDOR  "ITE     "   /* 8 byte vendor id */
#define SCSI1_VENDOR  "ITE     "   /* 8 byte vendor id */
#define SCSI2_VENDOR  "ITE     "   /* 8 byte vendor id */
#define SCSI3_VENDOR  "ITE     "   /* 8 byte vendor id */

/* 16 byte long product name. */
#define SCSI0_PRODUCT "Drive0          "  /* 16 byte product name */
#define SCSI1_PRODUCT "Drive1          "  /* 16 byte product name */
#define SCSI2_PRODUCT "Drive2          "  /* 16 byte product name */
#define SCSI3_PRODUCT "Drive3          "  /* 16 byte product name */

/* 4 byte long version number. */
#define SCSI0_VERSION "1.00"   /* 4 byte version number. */
#define SCSI1_VERSION "1.00"   /* 4 byte version number. */
#define SCSI2_VERSION "1.00"   /* 4 byte version number. */
#define SCSI3_VERSION "1.00"   /* 4 byte version number. */

const char  mst_info_text[4][29]  /* max. 4 luns */
= { SCSI0_VENDOR SCSI0_PRODUCT SCSI0_VERSION,
SCSI1_VENDOR SCSI1_PRODUCT SCSI1_VERSION,
SCSI2_VENDOR SCSI2_PRODUCT SCSI2_VERSION,
SCSI3_VENDOR SCSI3_PRODUCT SCSI3_VERSION };

//>>>>>>

void ithUsbdMstSetLun(uint32_t luns)
{
    N_LUNS_MST = luns;
}

void ithUsbdMstSetMediaTable(uint8_t idx, F_DRIVERINIT driver_init,
    uint32_t param, uint8_t iso)
{
    scsi_media_table_mst[idx].driver_init = driver_init;
    scsi_media_table_mst[idx].param = param;
    scsi_media_table_mst[idx].iso = iso;
}
