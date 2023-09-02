# A SPDK-based SSD Cache Framework 基於SPDK的固態硬碟快取框架

## 下載程式之後 (請先安裝好SPDK與fio)

1. 修改Makefile指向SPDK目錄
2. 修改inc/spdk.h的#define NVME_ADDR，改為綁定SPDK的device PCIE address
3. 可修改inc/config.h的參數

## examples

複製examples的code到testing.c，執行Make後，將編譯出執行檔app

## 執行fio

修改Makefile，並編譯後，需再修改job file(.fio)的ioengines位置

1. fio_engines為fio提供之ioengines
2. spdk_engines為SDPK提供之ioengines(SPDK將其稱為fio_plugin)
3. myspdk與為作者編寫之ioengines，用於測試spdk.c與spdk.h
4. ssd-cache-manager除執行fio測試外，還須自己維護mapping、migration worker等)
5. ssd-cache-client須配合examples/8_command_line執行(先啟動server，再執行ssd-cache-client的fio測試)

