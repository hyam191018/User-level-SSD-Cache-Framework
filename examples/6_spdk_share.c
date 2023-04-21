#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>  // fork

#include "shm.h"
#include "spdk.h"
#include "stdinc.h"

#define SHM_LOCK "/shm_lock"
#define PAGE_SIZE 4096

/**
 * @author Hyam
 * @date 2023/04/15
 * @brief 測試多用戶存取SPDK
 */

// 100w  1 user 44s
// 100w  3 user 130s
// 100w 10 user 422s
const int test_time = 30000;
const int user_number = 3;
const int max_lba = 1000;

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

    // 初始化SPDK
    printf("init_spdk\n");
    init_spdk();
    for (int i = 0; i < user_number; i++) {
        sem_post(&locks->sem_user);
    }

    sem_wait(&locks->sem_admin);
    printf("exit_spdk\n");
    exit_spdk();
}

static void user_func(void) {
    // 取得lock
    share_lock* locks = (share_lock*)link_shm(SHM_LOCK, sizeof(share_lock));

    // 等待user完成後，結束SPDK
    sem_wait(&locks->sem_user);
    printf("%d link_spdk\n", getpid());
    init_spdk();

    // user function
    void* buf = alloc_dma_buffer(PAGE_SIZE);
    unsigned lba;
    for (int i = 0; i < test_time; i++) {
        lba = rand() % max_lba;
        sprintf(buf, "I am %d, rw lba is %d", getpid(), lba);
        write_spdk(buf, lba, 8, MG_QUEUE);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8, MG_QUEUE);
    }
    free_dma_buffer(buf);
    printf("==================\n");
    printf("|   IO COMPLETE  |\n");
    printf("==================\n");

    // 結束
    exit_spdk();
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
