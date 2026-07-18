# YAFFS: Yet Another Flash File System. A NAND-flash specific file
# system.
#
# Copyright (C) 2002-2021 Aleph One Ltd.
#
# Created by Charles Manning <charles@aleph1.co.uk>
#
# This release of YAFFS is for commercial use by commercially licensed
# customers only. It may be used under the terms of your agreement with
# Aleph One Ltd. and is limited to use in products for which the
# customer is licensee, unless the customer has received an unlimited
# licence from Aleph One Ltd., in which case it may be used in any
# product.
# 
# In the event that this code is used outside the terms of a current,
# valid and paid agreement with Aleph One Ltd, the code is subject to
# the GPL version3 license under which the open source code is
# distributed.
#
# This release was created on 2025-04-18.
#
YDI_DIR = ../../../
YDI_FRAMEWORK_DIR = ../../

CLEAN_OBJS = test_1_yaffs_mount emfile-2k-0
TESTFILES = 	test_1_yaffs_mount.o  

all: test_1_yaffs_mount

YAFFS_TEST_OBJS = $(COMMONTESTOBJS) $(TESTFILES)


ALL_UNSORTED_OBJS += $(YAFFS_TEST_OBJS) $(FUZZER_OBJS)

include $(YDI_FRAMEWORK_DIR)/FrameworkRules.mk

phony. test: test_1_yaffs_mount
	./test_1_yaffs_mount

test_1_yaffs_mount: $(FRAMEWORK_SOURCES) $(YAFFS_TEST_OBJS)
	gcc $(CFLAGS) -o $@ $(YAFFS_TEST_OBJS) -lpthread

