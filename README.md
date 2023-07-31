# User-level SSD Cache Framework

## 安裝 (請先安裝好SPDK與fio)

```git clone https://github.com/hyam191018/User-level_SSD_Cache_Framework.git```

1. 修改Makefile指向SPDK目錄

2. 修改inc/spdk.h的#define NVME_ADDR，改為綁定SPDK的device PCIE address

3. 可修改inc/config.h的參數

## 執行examples

複製examples的code到testing.c，執行Make後，將編譯出執行檔app

## 執行fio

修改Makefile，並編譯後，需再修改job file(.fio)的ioengines位置

1. fio_engines為fio提供之ioengines

2. spdk_engines為SDPK提供之ioengines(SPDK將其稱為fio_plugin)

3. myspdk與為作者編寫之ioengines，用於測試spdk.c與spdk.h

4. ssd-cache-manager可直接執行(程式本身除執行fio測試外，還須自己維護mapping、migration worker等)

5. ssd-cache-client須配合examples/8_command_line執行，ssd-cache-client只需專注在fio測試上。

