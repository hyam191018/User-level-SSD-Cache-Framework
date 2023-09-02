![image](https://github.com/hyam191018/User-level-SSD-Cache-Framework/assets/59702782/2dd104a4-7c1a-45f8-8502-6a15e8cbe331)# User-level SSD Cache Framework 

## 下載程式之後 (請先安裝好SPDK與fio)

1. 修改Makefile指向SPDK目錄
2. 修改inc/spdk.h的#define NVME_ADDR，改為綁定SPDK的device PCIE address
3. 可修改inc/config.h的參數

## Examples

複製examples的code到testing.c，執行Make後，將編譯出執行檔app

## FIO

修改Makefile，並編譯後，需再修改job file(.fio)的ioengines位置

1. fio_engines為fio提供之ioengines
2. spdk_engines為SDPK提供之ioengines(SPDK將其稱為fio_plugin)
3. myspdk與為作者編寫之ioengines，用於測試spdk.c與spdk.h
4. ssd-cache-manager除執行fio測試外，還須自己維護mapping、migration worker等)
5. ssd-cache-client須配合examples/8_command_line執行(先啟動server，再執行ssd-cache-client的fio測試)

## 實驗結果

### 實驗環境

|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| CPU | Intel(R) Core(TM) i7-10700 CPU @ 2.90GHz |
| RAM  | Transcend 32GB DDR4 3200 MT/s |
| Origin device | TOSHIBA DT01ACA100 1TB |
| Cache device | WDS500G3X0C-00SJG0 500GB |
| OS | Ubuntu 18.04.6 LTS |
| Linux kernel | 5.4.0 |

### 基準測試

### 性能測試 1

### 性能測試 2



