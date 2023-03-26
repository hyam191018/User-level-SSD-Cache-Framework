#ifndef SHM_H
#define SHM_H

/*
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: 建立、連結、釋放、刪除共享記憶體位置
 */

#include "stdinc.h"

/*
 * Description: open and mmap to share memory
 * Return:  share memory address, if success
 *          NULL, if fail
 */
void* alloc_shm(char* shm_name, size_t shm_size);

/*
 * Description: mmap to share memory
 * Return:  share memory address, if success
 *          NULL, if fail
 */
void* link_shm(char* shm_name, size_t shm_size);

/*
 * Description: unmap from share memory
 * Return:  0, if success
 *          1, if fail
 */
int unmap_shm(void* shm_ptr, size_t shm_size);

/*
 * Description: unlink share memory
 * Return:  0, if success
 *          1, if fail
 */
int unlink_shm(char* shm_name);

#endif