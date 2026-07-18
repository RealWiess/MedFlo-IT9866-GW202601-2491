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
$(YAFFSLIBOBJS): %.o: %.c
	gcc -c $(CFLAGS)   -o $@ $<


$(YAFFSDIRECTSYMLINKS):
	ln -s ../$@ $@

$(DIRECTEXTRASYMLINKS):
	ln -s ../basic-test/$@ $@


# Makefile for stress tests and fuzzer
#
#
# YAFFS: Yet another Flash File System. A NAND-flash specific file system.
#
# Copyright (C) 2002-2018 Aleph One Ltd.
#
#
# Created by Charles Manning <charles@aleph1.co.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# NB Warning this Makefile does not include header dependencies.
#

YDI_DIR = ../../
YDI_FRAMEWORK_DIR = ../

TARGETS = libyaffsfs.so


all: $(TARGETS)


YAFFSLIBOBJS  = $(COMMONTESTOBJS) yaffs_python_helper.o

ALL_UNSORTED_OBJS += $(YAFFSLIBOBJS)

include ../FrameworkRules.mk

CFLAGS +=    -O0 -fPIC


libyaffsfs.so: $(FRAMEWORK_SOURCES) $(YAFFSLIBOBJS)
	gcc -shared $(YAFFSLIBOBJS) -o $@

