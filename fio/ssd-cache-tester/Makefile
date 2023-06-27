#  SPDX-License-Identifier: BSD-3-Clause
#  Copyright (c) Intel Corporation.
#  Copyright (c) 2015-2016, Micron Technology, Inc.
#  All rights reserved.
#

SPDK_ROOT_DIR := $(abspath $(CURDIR)/../../../spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

FIO_PLUGIN := ssd-cache-tester


# 自己修改 ssd-cache_4KB || ssd-cache_32KB
C_SRCS := \
ssd-cache-tester.c \
../../lib/pio.c \
../../lib/spdk.c \
../../lib/atomic.c \
../../lib/shm.c \
../../lib/work_queue.c \
../../lib/ssd_cache_api.c  \
../../lib/ssd_cache_target.c \
../../lib/ssd_cache_mapping.c \

CFLAGS += -I$(abspath $(CURDIR)/../../inc)
CFLAGS += -g -O2

SPDK_LIB_LIST = $(ALL_MODULES_LIST) event event_bdev

include $(SPDK_ROOT_DIR)/mk/spdk.fio.mk
