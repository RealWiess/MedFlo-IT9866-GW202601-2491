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
YDI_DIR = ../../../../
YDI_FRAMEWORK_DIR = ../../../

MAINFILES = time_32_tests create_32_bit validate_32_bit
MAIN_OBJS = $(addsuffix .o,$(MAINFILES))

EXTRA_OBJS = shared.o

CLEAN_OBJS = $(MAINFILES) 
CFLAGS = -DCONFIG_YAFFS_USE_32_BIT_TIME_T

YAFFS_TEST_OBJS = $(COMMONTESTOBJS) $(EXTRA_OBJS)
ALL_UNSORTED_OBJS += $(YAFFS_TEST_OBJS) $(MAIN_OBJS)

all: $(MAINFILES) 

include $(YDI_FRAMEWORK_DIR)/FrameworkRules.mk

phony. test: time_32_tests
	./time_32_tests

$(MAINFILES): $(FRAMEWORK_SOURCES) $(YAFFS_TEST_OBJS) $(MAIN_OBJS) 
	gcc $(CFLAGS) -o $@ $(YAFFS_TEST_OBJS) $@.o -lpthread -DCONFIG_YAFFS_USE_32_BIT_TIME_T

