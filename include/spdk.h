#ifndef SPDK_H
#define SPDK_H

/*
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 使用SPDK存取 cache device
 */

#include "config.h"

typedef struct {
    char *bdev_name;
    unsigned block_size;        // 通常是 512 Bytes
    unsigned long device_size;  // LBA的數量
} device_info;

device_info get_device_info(void);

#endif