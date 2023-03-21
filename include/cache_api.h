#ifndef CACHE_API_H
#define CACHE_API_H

/*
 * Author: Hyam
 * Date: 2023/03/19
 * Description: udm-cache的操作接口，包含建立、提交與刪除等
 */

#include "cache_type.h"
#include "config.h"
#include "pio.h"

/* in udm_cache_api.c */
/* return 0 if success */

/* admin: init > running > exit */
/* users" link > running > free */
int init_udm_cache(void);  // open and init
int link_udm_cache(void);  // opan and map
int free_udm_cache(void);  // unmap
int exit_udm_cache(void);  // unlink
void info_udm_cache(void);

int wakeup_mg_worker(void);
int shutdown_mg_worker(void);
int wakeup_wb_worker(void);
int shutdown_wb_worker(void);

int submit_pio(struct pio* pio);


#endif