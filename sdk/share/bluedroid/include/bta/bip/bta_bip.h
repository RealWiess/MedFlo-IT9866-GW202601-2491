/******************************************************************************
 *
 *  Copyright (C) 2014 The Android Open Source Project

 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "include/api/bd_bt_defs.h"

void bta_bip_open(bd_addr_t remote_bda);

void bta_bip_close(void);

void bta_bip_update_cover_art(uint8_t *cover_name);

//If pCoverArtPath is NULL , default cover path is "A:/cover_art.jpg"
//else if CFG_UPGRADE_LFS_FAT_ON_PARTITION0_AND_1 is opened , path will be CFG_TEMP_DRIVE":/cover_art.jpg"
void bta_bip_init(char* pCoverArtPath);

bool bta_is_cover_art_done(void);

bool bta_is_cover_art_connected(void);

void bta_bip_set_dump_coverArt(bool b_dump, char* pCoverArtPath);

typedef void (*BipCoverArtCallback)(uint8_t *image_data, size_t image_size, const char *mime);

void bta_bip_register_coverArt_callback(BipCoverArtCallback cb);

