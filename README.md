# User-level SSD Cache Framework

## 安裝 (請先安裝好SPDK與fio)

```git clone https://github.com/hyam191018/User-level_SSD_Cache_Framework.git```

1. 修改Makefile指向SPDK目錄

2. 修改inc/spdk.h的#define NVME_ADDR，改為綁定SPDK的device PCIE address

3. 可修改inc/config.h的參數

## 執行examples

複製examples的code到testing.c，執行Make後，將編譯出執行檔app

## 執行fio


