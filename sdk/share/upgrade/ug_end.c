#include <errno.h>
#include <stdio.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#pragma pack(1)
typedef struct
{
    uint32_t crc;
} end_t;

int ugUpgradeEnd(ITCStream *f)
{
    int ret = 0;
    end_t end;
    uint32_t readsize;

    // read end block
    readsize = itcStreamRead((ITCStream*)f, &end, sizeof(end_t));
    if (readsize != sizeof(end_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(end_t));
        goto end;
    }

    end.crc = itpLetoh32(end.crc);
    UG_LOG_DBG("crc=0x%X\n", end.crc);

end:
    return ret;
}
