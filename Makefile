SPDK_ROOT_DIR := $(abspath $(CURDIR)/../spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

APP = main

C_SRCS := \
./source/main.c \
./library/pio.c \
./library/atomic.c \
./library/udm_cache_api.c  \
./library/udm_cache_target.c \
./library/udm_cache_mapping.c 

CFLAGS += -I$(abspath $(CURDIR)/include)
CFLAGS += -g

SPDK_LIB_LIST = $(ALL_MODULES_LIST) event event_bdev

include $(SPDK_ROOT_DIR)/mk/spdk.app.mk
