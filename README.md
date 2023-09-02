# User-level SSD Cache Framework 

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

| CPU | RAM | Origin device | Cache device | OS | Linux kernel |
|:--:|:--:|:--:|:--:|:--:|:--:|
| Intel(R) Core(TM) i7-10700 CPU @ 2.90GHz | Transcend 32GB DDR4 3200 MT/s | TOSHIBA DT01ACA100 1TB | WDS500G3X0C-00SJG0 500GB | Ubuntu 18.04.6 LTS | 5.4.0 | 

### 基準測試


| Thread  | Direct IO | IO depth | File size | Run time | Block device | IO engine | File system |
|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| 1 | True | 1 | 1GB | 1 min | HDD/SSD/SSD | PSYNC/PSYNC/SPDK | EXT4/EXT4/NONE |

單位:IOPS
| operation/block size  | PSYNC HDD | PSYNC SSD | SPDK SSD | ratio |
|:--:|:--:|:--:|:--:|:--:|
| rand. read, 4K | 146 |  13.0K| 13.7K| 105%|
| rand. read, 4K  | 177| 64.0K | 84.8K | 132%|
| seq. read, 4K  | 16.3K| 35.1K | 39.6K| 112%|
| seq. read, 4K  | 14.6K | 64.1K| 84.8K| 132%|
| rand. read, 32K  | 140|7.2K |7.5K | 104%|
| rand. read, 32K  | 116| 38.1K| 44.9K| 117%|
| seq. read, 32K  | 5.6K| 20.2K| 20.6K| 101%|
| seq. read, 32K  | 4.8K| 38.2K| 44.9K| 117%|

### 性能測試 1 - 4GB SSD and 1GB File

單位:IOPS
| operation/block size  | dm-cache | USC Framework | ratio |
|:--:|:--:|:--:|:--:|
| rand. read, 4K | 9.6K |  13.6K| 141% |
| rand. read, 4K  |11.2K| 84.1K | 750% |
| seq. read, 32K  | 6.4K| 7.3K| 114% |
| seq. read, 32K  | 12.5K| 43.8K| 350% |

### 性能測試 2 - 4GB SSD and 8GB File

單位:IOPS, hit ratio
| operation/block size  | dm-cache | USC Framework | ratio |
|:--:|:--:|:--:|:--:|
| rand. read, 4K | 205, 48% |  188, 49%| 91% |
| rand. read, 4K  |225, 49%| 225, 49% | 113% |
| seq. read, 32K  |5.7K, 41%| 7.2K, 38%| 124% |
| seq. read, 32K  |1.4K, 47%| 3.1K, 50% | 221% |

