#ifndef CACHE_API_H
#define CACHE_API_H

/*
 *  Author: Hyam
 *  Date: 2023/03/19
 *  Description: udm-cache的操作接口，包含建立、提交與刪除等
 *              admin: init > running > exit
 *              users: link > running > free
 */

#include "cache_type.h"
#include "config.h"
#include "pio.h"

/*
 * Description: init share memory space, wakeup workers, called by admin
 * Return:  0, if success
 */
int init_udm_cache(void);

/*
 * Description: map to share memory space, called by admin
 * Return:  0, if success
 */
int link_udm_cache(void);

/*
 * Description: unmap from share memory space, called by user
 * Return:  0, if success
 */
int free_udm_cache(void);

/*
 * Description: unlink share memory space, called by admin
 * Return:  0, if success
 */
int exit_udm_cache(void);

/*
 * Description: force to unlink share memory, avoid lock (for debug)
 * Return:  No return value
 */
void force_exit_udm_cache(void);

/*
 * Description: print udm-cache mapping info
 * Return:  No return value
 */
void info_udm_cache(void);

/*
 * Description: submit a page io to udm-cache-target
 * Return:  0, if success
 */
int submit_pio(struct pio* pio);

#endif