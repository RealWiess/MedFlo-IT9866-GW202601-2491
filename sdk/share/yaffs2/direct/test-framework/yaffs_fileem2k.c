/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file
 * system.
 * 
 * Copyright (C) 2002-2021 Aleph One Ltd.
 * 
 * Created by Charles Manning <charles@aleph1.co.uk>
 * 
 * This release of YAFFS is for commercial use by commercially licensed
 * customers only. It may be used under the terms of your agreement with
 * Aleph One Ltd. and is limited to use in products for which the
 * customer is licensee, unless the customer has received an unlimited
 * licence from Aleph One Ltd., in which case it may be used in any
 * product.
 * 
 * In the event that this code is used outside the terms of a current,
 * valid and paid agreement with Aleph One Ltd, the code is subject to
 * the GPL version3 license under which the open source code is
 * distributed.
 * 
 * This release was created on 2025-04-18.
 * 
 */

/*
 * This provides a YAFFS nand emulation on a file for emulating 2kB pages.
 * This is only intended as test code to test persistence etc.
 */

#include "yportenv.h"
#include "yaffs_trace.h"

#include "yaffs_guts.h"
#include "yaffs_fileem2k.h"
#include "yaffs_packedtags2.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ite/ite_nand.h"
#include "itp_yaffs2.h"

extern unsigned char* itp_yaffs_nf_block_buffer;
extern int itp_yaffs_info_block[3];
extern int detect_reading_cycle_issue;

/*Bad block collect*/
//extern struct YaffsBadBlockCollect bbcollect;
//extern struct YaffsBadBlockCollect itpGetYaffsBadBlockCollect();
static struct YaffsBadBlockCollect bbcollect = {0, 0, NULL};

static int YaffsWriteBackupBlockInfo(struct yaffs_dev *dev, struct YaffsBackupBlockInfo* info)
{
	int i;
	int ret;
	u8 *temp = malloc((dev->param.total_bytes_per_chunk + dev->param.spare_bytes_per_chunk) * dev->param.chunks_per_block);
	int check_tag_count = 0;
	int YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR0] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
	int BACKUP_INFO_ADDRESS;
	int info_block, info_page;
	int new_info_block;
	
//read tag
READ_TAG:	
	ret=iteNfRead(
        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk), 
        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk, 
        temp);
    if (ret < 0) {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk),
        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk);

		if(temp) free(temp);
        return -1;
    }

	if (!memcmp(temp, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG)))
	{
		BACKUP_INFO_ADDRESS = YAFFS_FLASH_TAGADDRESS -2 * dev->param.total_bytes_per_chunk;
		info_block = BACKUP_INFO_ADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk);
		info_page = (BACKUP_INFO_ADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk;
    }
	else
	{
		check_tag_count++;
		if(check_tag_count > 1)
		{
			printf("%s(): not a valid info block\n", __FUNCTION__);
			if(temp) free(temp);
        	return YAFFS_FAIL;
		}
		YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR1] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
		goto READ_TAG;
	}	
	
    for(i = 0; i < dev->param.chunks_per_block; i++)
	{
		if(i != info_page)
		{
			ret = iteNfRead(
	            info_block,
	            i, 
	            temp+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk));
			if (ret < 0)
			{
	            ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, info_block, i);
				if(temp) free(temp);
	            return YAFFS_FAIL;
	        }
		}
		else
		{
			memcpy(temp+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
                info,
                sizeof(struct YaffsBackupBlockInfo));
		}
	}

	if(check_tag_count == 0)
	{
		new_info_block = itp_yaffs_info_block[YAFFS_INFO_ADDR1];
	}
	else
	{
		new_info_block = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
	}
	printf("new_info_block = %d\n", new_info_block);
	ret=iteNfErase(new_info_block);
    if (ret!=true) {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, new_info_block);
		if(temp) free(temp);
        return -1;
    }

	for(i = 0; i < dev->param.chunks_per_block; i++)
	{
		ret = iteNfWrite(
            new_info_block,
            i, 
            temp+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
            temp+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk)+dev->param.total_bytes_per_chunk);
		if (ret < 0)
		{
            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, new_info_block, i);
			if(temp) free(temp);
            return YAFFS_FAIL;
        }
	}
	
    ret=iteNfErase(info_block);
    if (ret!=true) {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, info_block);
		if(temp) free(temp);
        return -1;
    }

	if(temp)
		free(temp);
	return 0;
}

static int yflash2_WriteChunk(struct yaffs_dev *dev, int nand_chunk,
				   const u8 *data, int data_len,
				   const u8 *oob, int oob_len)
{
	u8 *buffer = dev->spare_buffer;
	u8 *e;
	int i;
	int block = nand_chunk/dev->param.chunks_per_block;
    int page = nand_chunk%dev->param.chunks_per_block;
	int ret;

	if(!data)
		return YAFFS_FAIL;

	/* Calc ECC and marshall the oob bytes into the buffer */
	memset(buffer, 0xff, dev->param.spare_bytes_per_chunk);
	if (oob)
	{
		memcpy(buffer, oob, oob_len);
	}

#if 0
	ithFlushDCacheRange((void*)data, data_len);
	ithFlushDCacheRange((void*)buffer, dev->param.spare_bytes_per_chunk);
	ithFlushMemBuffer();
	ithInvalidateDCacheRange((void*)data, data_len);
	ithInvalidateDCacheRange((void*)buffer, dev->param.spare_bytes_per_chunk);
#endif

	ret = iteNfWrite(block, page, (uint8_t *)data, buffer);
	if (ret < 0)
	{
		return YAFFS_FAIL;
	}
	else
	{	
	    return YAFFS_OK;
	}
}

static int yflash2_ReadChunk(struct yaffs_dev *dev, int nand_chunk,
				   u8 *data, int data_len,
				   u8 *oob, int oob_len,
				   enum yaffs_ecc_result *ecc_result_out)
{
	int ret = -1;
	int i;
	u8 *buffer = dev->ext_buffer;
	int block = nand_chunk/dev->param.chunks_per_block;
    int page = nand_chunk%dev->param.chunks_per_block;
	int retry = 0;
	int ecc_result = 0;
#if 1	
	do
	{
		int tempEccResult = 0;
		int tempRet = iteNfReadEcc(block, page, buffer, &tempEccResult);
		if(ret < tempRet)
		{
			ret = tempRet;
		}
		if(ecc_result < tempEccResult)
			ecc_result = tempEccResult;
		if(tempRet > 0)
			break;
		retry ++;
		ithPrintf("iteNfRead fail, retry %X %X %X %X %X %X %X\r\n", block, page, retry, tempRet, ret, tempEccResult, ecc_result);
		usleep(1);
	} while (retry < 3);
#else
	ret = iteNfRead(block, page, buffer);
#endif	
#if 0
	ithFlushDCacheRange((void*)buffer, dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk);
	ithFlushMemBuffer();
	ithInvalidateDCacheRange((void*)buffer, dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk);
#endif

	if (ret == 2)
	{
		int rv;
		//int BACKUP_BLOCK_ADDRESS = itp_yaffs_info_block[YAFFS_BACKUP_BLOCK_ADDR] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk);
		int backup_block = itp_yaffs_info_block[YAFFS_BACKUP_BLOCK_ADDR];

		struct YaffsBackupBlockInfo backupblockInfo;
		printf("Detect read cycle issue, need to rewrite block.\n");
		for(i = 0; i < dev->param.chunks_per_block; i++)
		{
			int retry2 = 0;
			while(retry2 < 3)
			{
				rv = iteNfRead(
					block,
					i, 
					itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk));
				if (rv < 0)
				{
					if (ecc_result_out)
						*ecc_result_out = YAFFS_ECC_RESULT_UNFIXED;

					ithPrintf("[X] %s():iteNfRead(%d, %d, %d)\n", __FUNCTION__, block, i, retry2);
					retry2 ++;
					usleep(1);
					//return YAFFS_FAIL;
				}
				else 
					break;	
			}
			if(retry2 >= 3)
			{
				memset(itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk), 0xFF, dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk);
				ithPrintf("%s iteNfRead(%d, %d, %d) retry NG\n", __FUNCTION__, block, i, retry2);
			}
		}

		//write backup block data
		rv = iteNfErase(backup_block);
		if (rv < 0) {
        	ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, backup_block);
        	return YAFFS_FAIL;
    	}

		for(i = 0; i < dev->param.chunks_per_block; i++)
		{
			rv = iteNfWrite(
	            backup_block,
	            i, 
	            itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
	            itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk)+dev->param.total_bytes_per_chunk);
			if (rv < 0)
			{
	            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, backup_block, i);
	            return YAFFS_FAIL;
	        }
		}

		//write backup block info, set need_recover flag to 1
		backupblockInfo.need_recover = 1;
		backupblockInfo.recover_block_index = block;
		YaffsWriteBackupBlockInfo(dev, &backupblockInfo);
		
		rv = iteNfErase(block);
		if (rv < 0) {
        	ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, block);
        	return YAFFS_FAIL;
    	}

		for(i = 0; i < dev->param.chunks_per_block; i++)
		{
			rv = iteNfWrite(
                block,
                i, 
                itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
                itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk)+dev->param.total_bytes_per_chunk);
			if (rv < 0)
			{
	            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, block, i);
	            return YAFFS_FAIL;
	        }
		}

		//write backup block info, set need_recover flag to 0
		backupblockInfo.need_recover = 0;
		backupblockInfo.recover_block_index = block;
		YaffsWriteBackupBlockInfo(dev, &backupblockInfo);
	}
	
	if(oob)
	{
		memcpy(oob, buffer + dev->param.total_bytes_per_chunk, oob_len);
#if 0		
		ithFlushDCacheRange(oob, oob_len);
		ithFlushMemBuffer();
		ithInvalidateDCacheRange((void*)oob, oob_len);
#endif		
	}

	if(data)
	{
		memcpy(data, buffer, data_len);
#if 0		
		ithFlushDCacheRange(data, data_len);
		ithFlushMemBuffer();
		ithInvalidateDCacheRange((void*)data, data_len);
#endif		
	}

	if (ret < 0)
	{		
		if (ecc_result_out)
			*ecc_result_out = YAFFS_ECC_RESULT_UNFIXED;
		return YAFFS_FAIL;
	}
	else
	{
		if (ecc_result_out)
			*ecc_result_out = ecc_result > dev->param.eccThr/2 ? YAFFS_ECC_RESULT_FIXED : YAFFS_ECC_RESULT_NO_ERROR;
		return YAFFS_OK;
	}
}

static int yflash2_EraseBlock(struct yaffs_dev *dev, int block_no)
{
	int ret;
	ret = iteNfErase(block_no);
	if(ret == -1)
		return YAFFS_FAIL;
	else
		return YAFFS_OK;
}

static int yflash2_MarkBad(struct yaffs_dev *dev, int block_no)
{
	int i;
	int ret;
	u8 *buffer = dev->ext_buffer;
	int check_tag_count = 0;
	int YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR0] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
	int YAFFS2_FLASH_BBTADDRESS;
	int bbt_block, bbt_page, bbt_mark_page;
	int bbt_index;
	int new_bbt_block;
	struct YaffsBadBlockTable bbtable;
	

	//read tag
READ_TAG:	
	ret=iteNfRead(
        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk), 
        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk, 
        buffer);
    if (ret < 0) {
        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk),
        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk);
        return -1;
    }

	if (!memcmp(buffer, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG)))
	{
		YAFFS2_FLASH_BBTADDRESS = YAFFS_FLASH_TAGADDRESS -(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
		bbt_block = YAFFS2_FLASH_BBTADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk);
		bbt_page = (YAFFS2_FLASH_BBTADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk;
		bbt_mark_page = bbt_page + (block_no -1)/MAX_BB_NUM_PER_PAGE;
    }
	else
	{
		check_tag_count++;
		if(check_tag_count > 1)
		{
			printf("%s(): not a valid info block\n", __FUNCTION__);
        	return YAFFS_FAIL;
		}
		YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR1] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
		goto READ_TAG;
	}

	for(i = 0; i < dev->param.chunks_per_block; i++)
	{
		if(i != bbt_mark_page)
		{
			ret = iteNfRead(
	            bbt_block,
	            i, 
	            itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk));
			if (ret < 0)
			{
	            ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, bbt_block, i);
	            return YAFFS_FAIL;
	        }
		}
		else
		{
			ret = iteNfRead(
	            bbt_block,
	            i, 
	            buffer);
			if (ret < 0)
			{
	            ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, bbt_block, i);
	            return YAFFS_FAIL;
	        }
			bbtable.bbt = malloc(sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
			memcpy(&(bbtable.bb_count), buffer, sizeof(uint32_t));
			memcpy(bbtable.bbt, buffer+sizeof(uint32_t), sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
			bbt_index = bbtable.bb_count;
			bbtable.bbt[bbt_index] = block_no;
			bbtable.bb_count++;
			
			memcpy(itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
                &(bbtable.bb_count), sizeof(uint32_t));
			memcpy(itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk)+sizeof(uint32_t),
				bbtable.bbt, sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
			if(bbtable.bbt)
				free(bbtable.bbt);	
		}
	}

	if(check_tag_count == 0)
	{
		new_bbt_block = itp_yaffs_info_block[YAFFS_INFO_ADDR1];
	}
	else
	{
		new_bbt_block = itp_yaffs_info_block[YAFFS_INFO_ADDR0];
	}

	ret=iteNfErase(new_bbt_block);
    if (ret!=true) {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, new_bbt_block);
        return -1;
    }

	for(i = 0; i < dev->param.chunks_per_block; i++)
	{
		ret = iteNfWrite(
            new_bbt_block,
            i, 
            itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk),
            itp_yaffs_nf_block_buffer+i*(dev->param.total_bytes_per_chunk+dev->param.spare_bytes_per_chunk)+dev->param.total_bytes_per_chunk);
		if (ret < 0)
		{
            ithPrintf("[X] %s():iteNfWrite(%d, %d)\n", __FUNCTION__, new_bbt_block, i);
            return YAFFS_FAIL;
        }
	}

	ret = iteNfErase(bbt_block);
    if (ret < 0) {
        ithPrintf("[X] %s():iteNfErase(%d)\n", __FUNCTION__, bbt_block);
        return YAFFS_FAIL;
    }

	if(bbcollect.is_bb_collected)
	{
		bbcollect.bb_buffer[bbcollect.bb_collect_count] = block_no;
		bbcollect.bb_collect_count++;
	}

	return YAFFS_OK;
}

static int yflash2_CheckBad(struct yaffs_dev *dev, int block_no)
{
	int i, j;
	//yflash2_MarkBad(dev, 1777);
	//yflash2_MarkBad(dev, 58);
	//yflash2_MarkBad(dev, 1024);

	if(!bbcollect.is_bb_collected)
	{
		int ret;
		int check_tag_count = 0;
		int YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR0] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
		int YAFFS2_FLASH_BBTADDRESS;
		int bbt_block, bbt_page;
		u8 *buffer = dev->ext_buffer;
		struct YaffsBadBlockTable bbtable;

		//read tag
READ_TAG:	
		ret=iteNfRead(
	        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk), 
	        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk, 
	        buffer);
	    if (ret < 0) {
	        ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__,
	        YAFFS_FLASH_TAGADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk),
	        (YAFFS_FLASH_TAGADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk);
	        return -1;
	    }
		else if(ret == 2)
		{
			printf("Detect read cycle issue\n");
			detect_reading_cycle_issue = 1;
		}
	
		if (!memcmp(buffer, ITP_YAFFS_TAG, strlen(ITP_YAFFS_TAG)))
		{
			YAFFS2_FLASH_BBTADDRESS = YAFFS_FLASH_TAGADDRESS -(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
			bbt_block = YAFFS2_FLASH_BBTADDRESS/(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk);
			bbt_page = (YAFFS2_FLASH_BBTADDRESS%(dev->param.chunks_per_block * dev->param.total_bytes_per_chunk))/dev->param.total_bytes_per_chunk;
	    }
		else
		{
			check_tag_count++;
			if(check_tag_count > 1)
			{
				printf("%s(): not a valid info block\n", __FUNCTION__);
	        	return YAFFS_FAIL;
			}
			YAFFS_FLASH_TAGADDRESS = itp_yaffs_info_block[YAFFS_INFO_ADDR1] * (dev->param.chunks_per_block * dev->param.total_bytes_per_chunk)+(dev->param.chunks_per_block-1)*dev->param.total_bytes_per_chunk;
			goto READ_TAG;
		}

		if (!bbcollect.bb_buffer) {
			bbcollect.bb_buffer = (uint32_t *)malloc(sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE * MAX_BBT_NUM);
			//printf("bbcollect.bb_buffer = 0x%x\n", bbcollect.bb_buffer);
		}
		
		bbcollect.bb_collect_count = 0;
		for(i = 0; i < MAX_BBT_NUM; i++)
		{
			ret = iteNfRead(
	            bbt_block,
	            bbt_page + i, 
	            buffer);
			if (ret < 0)
			{
	            ithPrintf("[X] %s():iteNfRead(%d, %d)\n", __FUNCTION__, bbt_block, bbt_page + i);
	            return YAFFS_FAIL;
	        }
			else if(ret == 2)
			{
				printf("Detect read cycle issue\n");
				detect_reading_cycle_issue = 1;
			}

			bbtable.bbt = malloc(sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
			memcpy(&(bbtable.bb_count), buffer, sizeof(uint32_t));
			memcpy(bbtable.bbt, buffer+sizeof(uint32_t), sizeof(uint32_t) * MAX_BB_NUM_PER_PAGE);
			//printf("YC: %s, %d, bbtable.bb_count = %d\n", __FUNCTION__, __LINE__, bbtable.bb_count);
			if(bbtable.bb_count > 0)
			{
				for(j = 0; j < bbtable.bb_count; j++)
				{
					bbcollect.bb_buffer[bbcollect.bb_collect_count + j] = bbtable.bbt[j];
				}
				bbcollect.bb_collect_count += bbtable.bb_count;
			}
			if(bbtable.bbt)
				free(bbtable.bbt);
		}
		bbcollect.is_bb_collected = true;
		
		if (detect_reading_cycle_issue == 1)
		{
			YaffsRewriteInfoBlock(check_tag_count);
			detect_reading_cycle_issue = 0;
		}
	}

	for(i = 0; i < bbcollect.bb_collect_count; i++)
	{
		//printf("bbcollect.bb_buffer[%d] = %d\n", i, bbcollect.bb_buffer[i]);
		if(bbcollect.bb_buffer[i] == block_no)
		{
			return YAFFS_FAIL;
		}
	}

	return YAFFS_OK;
}

static int yflash2_Initialise(struct yaffs_dev *dev)
{
	if (!dev->spare_buffer)
	{
	    dev->spare_buffer = malloc(dev->param.spare_bytes_per_chunk);
	}
	if (!dev->ext_buffer)
	{
	    dev->ext_buffer = malloc(dev->param.total_bytes_per_chunk + dev->param.spare_bytes_per_chunk);
	}
	return YAFFS_OK;
}

static int yflash2_Deinitialise(struct yaffs_dev *dev)
{
	if (dev->spare_buffer)
		free(dev->spare_buffer);
	dev->spare_buffer = NULL;

	if (dev->ext_buffer)
		free(dev->ext_buffer);
	dev->ext_buffer = NULL;
	return YAFFS_OK;
}

struct yaffs_dev *yflash2_install_drv(
				const char *name, 
				int pages_per_block,
				int data_bytes_per_page,
				int spare_bytes_per_page, 
				int start_block, 
				int end_block)
{
	struct yaffs_dev *dev = NULL;
	struct yaffs_param *param;
	struct yaffs_driver *drv;

	if(!bbcollect.is_bb_collected)
	{
		memset(&bbcollect, 0, sizeof(bbcollect));
	}

	dev = malloc(sizeof(*dev));

	if(!dev)
		return NULL;

	memset(dev, 0, sizeof(*dev));

	dev->param.name = strdup(name);

	if(!dev->param.name) {
		free(dev);
		return NULL;
	}

	drv = &dev->drv;

	drv->drv_write_chunk_fn = yflash2_WriteChunk;
	drv->drv_read_chunk_fn = yflash2_ReadChunk;
	drv->drv_erase_fn = yflash2_EraseBlock;
	drv->drv_mark_bad_fn = yflash2_MarkBad;
	drv->drv_check_bad_fn = yflash2_CheckBad;
	drv->drv_initialise_fn = yflash2_Initialise;
	drv->drv_deinitialise_fn = yflash2_Deinitialise;

	param = &dev->param;

	param->total_bytes_per_chunk = data_bytes_per_page;
	param->chunks_per_block = pages_per_block;
	param->spare_bytes_per_chunk = spare_bytes_per_page;
	param->start_block = start_block;
	param->end_block = end_block;
	param->is_yaffs2 = 1;
	param->use_nand_ecc=1;
	param->inband_tags = 1;

	param->n_reserved_blocks = 5;
	param->wide_tnodes_disabled=0;
	param->refresh_period = 1000;
	param->n_caches = 10; // Use caches

	param->enable_xattr = 1;
	param->no_tags_ecc = 1;
	param->always_check_erased = CFG_YAFFS2_WRITE_CHECK_LEVEL;
#if defined(CFG_YAFFS2_LAZY_GC)
	param->lazy_gc = 1;
#else
	param->lazy_gc = 0;
#endif
	param->eccThr = iteNfGetEccThr();
	if(param->always_check_erased > YAFFS_CHECK_LEVEL_ALL)
	{
		free(dev);
		printf("%s(): not valid check level : %d\n", __FUNCTION__, param->always_check_erased);
		return NULL;
	}
	/* dev->driver_context is not used by this simulator */

	yaffs_add_device(dev);

	return dev;
}
