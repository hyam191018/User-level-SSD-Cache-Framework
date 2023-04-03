#  SPDX-License-Identifier: BSD-3-Clause
#  Copyright (c) Intel Corporation.
#  Copyright (c) 2015-2016, Micron Technology, Inc.
#  All rights reserved.
#

SPDK_ROOT_DIR := $(abspath $(CURDIR)/../../../spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

FIO_PLUGIN := myfio

C_SRCS := \
myfio.c \
../../library/pio.c \
../../library/spdk.c \
../../library/atomic.c \
../../library/shm.c \
../../library/work_queue.c \
../../library/udm_cache_api.c  \
../../library/udm_cache_target.c \
../../library/udm_cache_mapping.c \

CFLAGS += -I$(abspath $(CURDIR)/../../include)
CFLAGS += -g -O2

SPDK_LIB_LIST = $(ALL_MODULES_LIST) event event_bdev

include $(SPDK_ROOT_DIR)/mk/spdk.fio.mk

#sudo /home/hyam/fio/fio /home/hyam/udm-cache/fio/myudm-cache/myfio.fio