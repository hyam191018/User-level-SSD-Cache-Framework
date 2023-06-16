#  SPDX-License-Identifier: BSD-3-Clause
#  Copyright (c) Intel Corporation.
#  Copyright (c) 2015-2016, Micron Technology, Inc.
#  All rights reserved.
#

SPDK_ROOT_DIR := $(abspath $(CURDIR)/../../../spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

FIO_PLUGIN := ssd-cache

C_SRCS := \
fio_plugin.c \
../../library/pio.c \
../../library/spdk.c \
../../library/atomic.c \
../../library/shm.c \
../../library/work_queue.c \
../../library/ssd_cache_api.c  \
../../library/ssd_cache_target.c \
../../library/ssd_cache_mapping.c \

CFLAGS += -I$(abspath $(CURDIR)/../../include)
CFLAGS += -g -O2

SPDK_LIB_LIST = $(ALL_MODULES_LIST) event event_bdev

include $(SPDK_ROOT_DIR)/mk/spdk.fio.mk
