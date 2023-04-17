# User-space Device Mapper Cache

## Introduction



## Installation
請先參考[FIO](https://github.com/axboe/fio)，[SPDK安裝步驟](https://spdk.io/doc/getting_started.html)

在root底下執行下列指令，確認裝置已綁定SPDK

`[spdk]/scripts/setup.sh`

取得裝置資訊，並產生出config file，其中traddr為裝置位址

`[spdk]/scripts/gen_nvme.sh --json-with-subsystems > bdev.json`

## Setup
請先修改，將NVME_ADDR改成自己的目標裝置位址

`[udm-cache-v3]/include/spdk.h`

修改其他參數(如cache大小)

`[udm-cache-v3]/include/config.h`

## fio

修改Makefile內spdk的路徑，執行make後，udm-cache的fio engine會出現在spdk的資料夾內

`[spdk]/build/fio/udm-cache`

修改full_bench.fio內engine的路徑

`ioengine=[spdk]/build/fio/udm-cache`

執行fio測試

`$ sudo [fio] [udm-cache-v3]/fio/udm-cache/full_bench.fio`

## examples

### 1_create_pio 建立、釋放page io

### 2_work_queue 模擬cache miss提交與接收promotion請求

### 3_spdk_rw 以raw IO的方式，使用SPDK讀寫Nvme SSD

### 4_submit_pio_4K 模擬user(page cache)發送submit_pio指令

### 5_submit_pio_32K 模擬user(page cache)發送submit_pio指令，在write miss時，直接寫到SSD(optimaizable write)

### 6_spdk_share 模擬mult-process同時向SPDK提交IO請求

### 7_udmc_share 模擬mult-process同時使用udm-cache




### udmc_share.c

