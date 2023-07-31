# User-level SSD Cache

## Run

請修改Makefile指向SPDK目錄

再修改inc/spdk.h的 #define NVME_ADDR "0000:04:00.0"

然後到inc/config.h設置參數

最後複製examples的code到main，執行Make


