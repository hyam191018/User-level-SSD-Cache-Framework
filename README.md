# User-level SSD Cache Framework

## 安裝 (請先安裝好SPDK與fio)

## 執行examples

1. 修改Makefile指向SPDK目錄

2. 修改inc/spdk.h的#define NVME_ADDR，改為綁定SPDK的device PCIE address

3. 可修改inc/config.h的參數

4. 複製examples的code，並建立一個main.c，再執行Make

## 執行fio


