#include "ite/itp.h"

#define MAX_BB_NUM_PER_PAGE     500
#define MAX_BBT_NUM				60

#define ITP_YAFFS_TAG         		"yaffssffay"
#define ITP_YAFFS_RESERVE_BLOCKS  	6

enum {
  YAFFS_INFO_ADDR0 = 0,
  YAFFS_INFO_ADDR1 = 1,
  YAFFS_BACKUP_BLOCK_ADDR = 2,
};

struct YaffsPartition {
	uint32_t	blocks;
    uint32_t    block_start;
    uint32_t    block_end;
};

struct YaffsPartitionTable {
	//char                     tag[16];
	int 					 par_count;
    struct YaffsPartition    partitions[ITP_MAX_PARTITION];
};

struct YaffsBadBlockTable {
	uint32_t    bb_count;
	uint32_t    *bbt;
};

struct YaffsBadBlockCollect {
	uint8_t 	is_bb_collected;
	uint32_t 	bb_collect_count;
	uint32_t 	*bb_buffer;
};

struct YaffsBackupBlockInfo {
	uint8_t 	need_recover;
	uint32_t	recover_block_index;
};

int YaffsRewriteInfoBlock(int check_tag_count);

