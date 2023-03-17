SPDK_ROOT_DIR := $(abspath $(CURDIR)/../spdk)
include $(SPDK_ROOT_DIR)/mk/spdk.common.mk
include $(SPDK_ROOT_DIR)/mk/spdk.modules.mk

APP = main

C_SRCS := ./source/main.c ./library/udm_cache.c ./library/pio_api.c ./library/target.c ./library/mapping.c ./library/atomic.c

CFLAGS += -I$(abspath $(CURDIR)/include)

SPDK_LIB_LIST = $(ALL_MODULES_LIST) event event_bdev

include $(SPDK_ROOT_DIR)/mk/spdk.app.mk
