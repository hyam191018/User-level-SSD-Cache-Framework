#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

#define USERS 3       // 使用者數量
#define ROUND 100000  // 使用者submit次數
#define EXCEPT 70     // 期望的 hit ratio
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;

static int sem_id;

// 製作pio
static void send_pio(void) {
    char* name = "test";
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 8 - page_index % 8;
    struct pio* head = create_pio(name, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

static void* user_func(void* arg) {
    for (int i = 0; i < ROUND; i++)
        send_pio();
    return NULL;
}

static void user(void) {
    // 等待init完成
    struct sembuf sem_ops = {0, -1, 0};
    if (semop(sem_id, &sem_ops, 1) == -1) {
        perror("semop");
        exit(1);
    }

    // 使用者程式
    printf("%d link rc = %d\n", getpid(), link_udm_cache());

    printf("%d free rc = %d\n", getpid(), free_udm_cache());

    // 告知admin
    sem_ops.sem_op = 1;
    if (semop(sem_id, &sem_ops, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

static void admin(void) {
    // Initialize cache
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    // 告知user init完成
    struct sembuf sem_ops = {0, 1, 0};
    if (semop(sem_id, &sem_ops, 1) == -1) {
        perror("semop");
        exit(1);
    }

    // 等user結束
    sem_ops.sem_op = -1;
    if (semop(sem_id, &sem_ops, 1) == -1) {
        perror("semop");
        exit(1);
    }
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
}

int main(int argc, char* argv[]) {
    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_id, 0, SETVAL, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    pid_t pid_admin = fork();
    if (pid_admin == -1) {
        perror("fork");
        exit(1);
    } else if (pid_admin == 0) {
        admin();
        exit(0);
    }

    pid_t pid_user = fork();
    if (pid_user == -1) {
        perror("fork");
        exit(1);
    } else if (pid_user == 0) {
        user();
        exit(0);
    }

    wait(NULL);
    wait(NULL);

    if (semctl(sem_id, 0, IPC_RMID, 0) == -1) {
        perror("semctl");
        exit(1);
    }

    return 0;
}