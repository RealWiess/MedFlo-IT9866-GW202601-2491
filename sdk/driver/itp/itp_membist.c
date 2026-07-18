/*
 * Copyright (c) 2022 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL MEM BIST functions.
 *
 * @author
 * @version 1.0
 */
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include "itp_cfg.h"

#define BIST_TEST_SIZE 0x80000
#define DMA_TEST_SIZE  0x80000


static pthread_t MemBist_t;
static pthread_t MemDma_t;

static bool MBQuit = true;


static void* MemBistTask(void* arg)
{
	uint32_t test_mem_addr0 = itpVmemAlloc(BIST_TEST_SIZE);
	if(!test_mem_addr0)
	{
		printf("allocate bist 0 memory fail\n");
		return NULL;
	}

	uint32_t test_mem_addr1 = itpVmemAlloc(BIST_TEST_SIZE);
	if(!test_mem_addr1)
	{
		printf("allocate bist 1 memory fail\n");
		return NULL;
	}	

	printf("Allocate mem addr = 0x%x, 0x%x \n",test_mem_addr0, test_mem_addr1);

    while (!MBQuit)
    {

		//printf("mem bist 0 disable\n");
		ithMEMBISTDisable(0);
		ithMEMBIST_SET_STARTADD(0, test_mem_addr0);
		ithMEMBIST_SET_ENDADD(0, test_mem_addr0 + BIST_TEST_SIZE);
		ithMEMBIST_SET_MODE(0, ITH_MEMBIST_RDARB);
		ithMEMBIST_WRITE_LOWBIT(0, 0x0F0F0F0F);
		ithMEMBIST_WRITE_HIGHBIT(0, 0x0F0F0F0F);
		
		//printf("mem bist 1 disable\n");
		ithMEMBISTDisable(1);
		ithMEMBIST_SET_STARTADD(1, test_mem_addr1);
		ithMEMBIST_SET_ENDADD(1, test_mem_addr1 + BIST_TEST_SIZE);
		ithMEMBIST_SET_MODE(1, ITH_MEMBIST_RDARB);
		ithMEMBIST_WRITE_LOWBIT(1, 0x0F0F0F0F);
		ithMEMBIST_WRITE_HIGHBIT(1, 0x0F0F0F0F);

		ithMEMBISTEnable(0);
		
		ithMEMBISTEnable(1);

		while(1)
		{
			if(ithMEMBIST_ISDone(0) && ithMEMBIST_ISDone(1))
			{
				//printf("mem bist done\n");
				break;
			}

			if(ithMEMBIST_ISFault(0))
			{
				printf("mem bist 0 error\n");
			}
			
			if(ithMEMBIST_ISFault(1))
			{
				printf("mem bist 1 error\n");
			}			
			usleep(10);
		}
				
    }
	if(test_mem_addr0)
		itpVmemFree(test_mem_addr0);
	if(test_mem_addr1)
		itpVmemFree(test_mem_addr1);

    return NULL;
}
#if CFG_DMA_TEST_ENABLE
static void* MemDmaTask(void* arg)
{
	uint32_t srcAddr = 0, dstAddr = 0, i = 0;
	uint32_t size_alloc = DMA_TEST_SIZE;
	uint8_t* mapAddr = NULL;

	
    while (!MBQuit)
    {		
		srcAddr = itpVmemAlignedAlloc(64, size_alloc);
		if(!srcAddr)
		{
			printf(" allocate src memory fail! \n");
			return NULL;
		}
		dstAddr = itpVmemAlignedAlloc(64, size_alloc);
		if(!dstAddr)
		{
			printf(" allocate dst memory fail! \n");
			return NULL;
		}

		// prepare source data
		mapAddr = (uint8_t*)ithMapVram(srcAddr, size_alloc, ITH_VRAM_WRITE);
		for (i = 0; i < size_alloc; i++)
			mapAddr[i] = (uint8_t)((i) % 0x100);
		ithUnmapVram(mapAddr, size_alloc);

		// fill dst data with 0x0
		mapAddr = (uint8_t*)ithMapVram(dstAddr, size_alloc, ITH_VRAM_WRITE);
		memset(mapAddr, 0x0, size_alloc);
		ithUnmapVram(mapAddr, size_alloc);

		ithDmaMemcpy((dstAddr), (srcAddr), size_alloc);
		//printf("DMA cpy src = 0x%x, dst = 0x%x\n",srcAddr, dstAddr);

		// compare data
		{
			uint8_t* srcMap = (uint8_t*)ithMapVram(srcAddr, size_alloc, ITH_VRAM_READ);
			uint8_t* dstMap = (uint8_t*)ithMapVram(dstAddr, size_alloc, ITH_VRAM_READ);
			uint8_t* srcMap1 = srcMap;
			uint8_t* dstMap1 = dstMap;

			for (i = 0; i < DMA_TEST_SIZE; i++)
			{
				if(srcMap1[i] != dstMap1[i])
				{
					printf("[dma error] src[%X] = %02X, dst[%X] = %02X \n", i, srcMap1[i], i, dstMap1[i]);
				}
			}

			ithUnmapVram(srcMap, size_alloc);
			ithUnmapVram(dstMap, size_alloc);
		}

		if(srcAddr)
			itpVmemFree(srcAddr);
		if(dstAddr)
			itpVmemFree(dstAddr);
		
		usleep(10*1000);
	
    }

	return NULL;
}
#endif
static int MemBistIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
    case ITP_IOCTL_INIT:
		
		MBQuit = false;
		
        break;
		
	case ITP_IOCTL_EXIT:
		
		if(!MBQuit)
		{
			MBQuit = true;
			pthread_join(MemBist_t, NULL);
			#if CFG_DMA_TEST_ENABLE
			pthread_join(MemDma_t, NULL);
			#endif
		}

		break;

	case ITP_IOCTL_INIT_TASK:
		pthread_create(&MemBist_t, NULL, MemBistTask, NULL);
		#if CFG_DMA_TEST_ENABLE
		pthread_create(&MemDma_t, NULL, MemDmaTask, NULL);
		#endif
		break;
		
    default:
        errno = (ITP_DEVICE_MEMBIST << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceMemBist =
{
    ":membist",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    MemBistIoctl,
    NULL
};
