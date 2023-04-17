# User-space Device Mapper Cache

## Introduction



## Installation
請先參考[FIO](https://github.com/axboe/fio)，[SPDK安裝步驟](https://spdk.io/doc/getting_started.html)

在root底下執行下列指令，確認裝置已綁定SPDK

`spdk/scripts/setup.sh`

取得裝置資訊，並產生出config file，其中traddr為裝置位址

`spdk/scripts/gen_nvme.sh --json-with-subsystems > bdev.json`

## Setup
請先修改udm-cache-v3/include/spdk.h，將NVME_ADDR改成自己的目標裝置

`#define NVME_ADDR "0000:04:00.0"`

修改其他參數(如cache大小)

`udm-cache/include/config.h`

## fio
執行fio測試

`$ sudo [fio] [udm-cache]/fio/udm-cache/full_bench.fio`

## examples

### udmc_share.c

