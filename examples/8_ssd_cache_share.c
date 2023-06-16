#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>  // fork

#include "cache_api.h"
#include "shm.h"
#include "spdk.h"
#include "stdinc.h"

#define SHM_LOCK "/shm_lock"

/**
 * @author Hyam
 * @date 2023/04/15
 * @brief 測試多用戶存取ssd-cache
 */

//  1  user 100w  50%     299s
//  1  user 100w 100%      64s
//  1  user 100w 200%      55s
//  2  user  50w  50%     461s
//  5  user  20w  50%  10.5min
// 10  user  10w  50%    23min
//  5  user 100w  25%  65.5min
const int test_time = 30000;
const int user_number = 3;

#define EXCEPT 25  // 期望的 hit ratio
#define FILE_NAME "testfile"
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT * 4ull);

// 隨機4K讀寫
static void send_pio(void* buffer) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    if (operation == WRITE) {
        memset(buffer, 'A' + rand() % 25, PAGE_SIZE);
    }
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(FILE_NAME, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
}

typedef struct {
    pthread_mutex_t lock;
    sem_t sem_admin;
    sem_t sem_user;
    int user_count;
    bool isready;
} share_lock;

static void init_lock(share_lock* locks) {
    pthread_mutex_init(&locks->lock, NULL);
    locks->user_count = 0;
    sem_init(&locks->sem_admin, 1, 0);
    sem_init(&locks->sem_user, 1, 0);
}

static void exit_lock(share_lock* locks) {
    pthread_mutex_destroy(&locks->lock);
    sem_destroy(&locks->sem_admin);
    sem_destroy(&locks->sem_user);
}

static void admin_func(void) {
    // 取得lock
    share_lock* locks = (share_lock*)link_shm(SHM_LOCK, sizeof(share_lock));

    // 初始化ssd-cache
    force_exit_ssd_cache();
    printf("init_ssd_cache, rc = %d\n", init_ssd_cache());
    for (int i = 0; i < user_number; i++) {
        sem_post(&locks->sem_user);
    }

    // 結束ssd-cache
    sem_wait(&locks->sem_admin);
    info_ssd_cache();
    printf("exit_ssd_cache, rc = %d\n", exit_ssd_cache());
}

static void user_func(void) {
    // 取得lock
    share_lock* locks = (share_lock*)link_shm(SHM_LOCK, sizeof(share_lock));

    // 等待admin完成
    sem_wait(&locks->sem_user);
    printf("%d link_ssd_cache, rc = %d\n", getpid(), link_ssd_cache());

    // user function
    void* buffer = alloc_dma_buffer(PAGE_SIZE);
    for (int i = 0; i < test_time; i++) {
        send_pio(buffer);
    }
    free_dma_buffer(buffer);

    // 結束
    printf("%d unlink_ssd_cache, rc = %d\n", getpid(), unlink_ssd_cache());
    pthread_mutex_lock(&locks->lock);
    locks->user_count++;
    if (locks->user_count == user_number) {
        sem_post(&locks->sem_admin);
    }
    pthread_mutex_unlock(&locks->lock);
}

int main(int argc, char* argv[]) {
    // 建立share lock
    share_lock* locks = alloc_shm(SHM_LOCK, sizeof(share_lock));
    init_lock(locks);

    // 建立admin與users
    pid_t users[user_number], admin;

    admin = fork();
    if (admin == 0) {
        admin_func();
        exit(0);
    } else if (admin < 0) {
        fprintf(stderr, "Failed to fork process.\n");
        exit(1);
    }

    for (int i = 0; i < user_number; i++) {
        users[i] = fork();
        if (users[i] == 0) {
            user_func();
            exit(0);
        } else if (users[i] < 0) {
            fprintf(stderr, "Failed to fork process.\n");
            exit(1);
        }
    }

    // 等待子程序完成
    for (int i = 0; i < user_number + 1; i++) {
        wait(NULL);
    }
    exit_lock(locks);
    unlink_shm(SHM_LOCK);

    return 0;
}
