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
 * This code implements the ECC algorithm used in SmartMedia.
 *
 * The ECC comprises 22 bits of parity information and is stuffed into 3 bytes.
 * The two unused bit are set to 1.
 * The ECC can correct single bit errors in a 256-byte page of data.
 * Thus, two such ECC blocks are used on a 512-byte NAND page.
 *
 */

#ifndef __YAFFS_ECC_H__
#define __YAFFS_ECC_H__

struct yaffs_ecc_other {
	unsigned char col_parity;
	unsigned line_parity;
	unsigned line_parity_prime;
};

void yaffs_ecc_calc(const unsigned char *data, unsigned char *ecc);
int yaffs_ecc_correct(unsigned char *data, unsigned char *read_ecc,
		      const unsigned char *test_ecc);

void yaffs_ecc_calc_other(const unsigned char *data, unsigned n_bytes,
			  struct yaffs_ecc_other *ecc);
int yaffs_ecc_correct_other(unsigned char *data, unsigned n_bytes,
			    struct yaffs_ecc_other *read_ecc,
			    const struct yaffs_ecc_other *test_ecc);
#endif
