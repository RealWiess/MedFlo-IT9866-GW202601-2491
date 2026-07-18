#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#pragma pack(1)
typedef struct
{
    uint32_t device_type;
    uint32_t unformatted_size;
    uint32_t partition_count;
} device_t;

typedef enum
{
    DEVICE_NAND = 0,
    DEVICE_NOR  = 1,
    DEVICE_SD0  = 2,
    DEVICE_SD1  = 3,

    DEVICE_COUNT
} DeviceType;

static const char* device_table[] =
{
    "NAND",
    "NOR",
    "SD0",
    "SD1"
};

#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
extern unsigned long g_pkg_reserved_size;
#endif

int ugUpgradeDevice(ITCStream *f, UGDevice* device)
{
    int i, ret = 0;
    device_t dev;
    uint32_t readsize;

    // read device header
    readsize = itcStreamRead((ITCStream*)f, &dev, sizeof(device_t));
    if (readsize != sizeof(device_t))
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(device_t));
        goto end;
    }

    device->device_type         = itpLetoh32(dev.device_type);
    device->unformatted_size    = itpLetoh32(dev.unformatted_size);
    device->partition_count     = itpLetoh32(dev.partition_count);

    UG_LOG_INFO("Upgrade %s. Non-partition size is %ld bytes, and having %ld partition(s)\n",
        device_table[device->device_type],
        device->unformatted_size,
        device->partition_count
   );

	#ifdef CFG_ALLOW_CHANGE_RESERVED_SIZE
	UG_LOG_INFO( "got PKG's reserved size: %x\n", device->unformatted_size );
	g_pkg_reserved_size = device->unformatted_size;
	#else
		#ifdef CFG_NAND_RESERVED_SIZE
        if (device->device_type == DEVICE_NAND)
		{
			uint32_t nfRsvSize = (uint32_t)CFG_NAND_RESERVED_SIZE;

			if(nfRsvSize != 0u)
			{
				uint32_t pkgRsvSize = (uint32_t)device->unformatted_size;
				if(nfRsvSize != pkgRsvSize)
				{
					UG_LOG_ERR("NAND's reserved size is different: ori=%x, new=%x\n", nfRsvSize, pkgRsvSize);
					goto end;
				}
			}
		}
		#endif
	#endif

    readsize = itcStreamRead((ITCStream*)f, &device->size, sizeof(uint64_t) * device->partition_count);
    if (readsize != sizeof(uint64_t) * device->partition_count)
    {
        ret = __LINE__;
        UG_LOG_ERR("Cannot read file: %ld != %ld\n", readsize, sizeof(uint64_t) * device->partition_count);
        goto end;
    }

    for (i = 0; i < (int)device->partition_count; i++)
    {
        device->size[i] = itpLetoh64(device->size[i]);

        if (device->size[i] > 0)
            UG_LOG_INFO("Partition[%d] size is %lld\n", i, device->size[i]);
        else
            UG_LOG_INFO("Partition[%d] size is maximum availiable\n", i);
    }

    if (device->device_type == DEVICE_NAND)
    {
        device->disk = ITP_DISK_NAND;
    }
    else if (device->device_type == DEVICE_NOR)
    {
        device->disk = ITP_DISK_NOR;
    }
    else if (device->device_type == DEVICE_SD0)
    {
        device->disk = ITP_DISK_SD0;
    }
    else if (device->device_type == DEVICE_SD1)
    {
        device->disk = ITP_DISK_SD1;
    }
    else
    {
        ret = __LINE__;
        UG_LOG_ERR("Unknown device type: %d\n", device->device_type);
        goto end;
    }

end:
#ifdef _WIN32
    ret = 0;
#endif
    return ret;
}
