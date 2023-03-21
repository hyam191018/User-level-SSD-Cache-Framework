#include "cache_api.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

static void function(void)
{
    printf("%d link rc %d\n", getpid(), link_udm_cache());
    printf("%d free rc %d\n", getpid(), free_udm_cache());
}

int main(int argc, char *argv[])
{
    int num_processes = 10;   // 指定建立的子進程數量
    pid_t pid[num_processes]; // 宣告子進程pid的陣列

    // force_exit_udm_cache();
    printf("%d init rc %d\n", getpid(), init_udm_cache());

    // 使用迴圈建立多個子進程
    for (int i = 0; i < num_processes; i++)
    {
        pid[i] = fork();
        if (pid[i] < 0)
        {
            fprintf(stderr, "Fork failed.\n");
            exit(1);
        }
        else if (pid[i] == 0)
        {
            function();
            exit(0);
        }
    }

    // 父進程等待所有子進程結束
    for (int i = 0; i < num_processes; i++)
    {
        waitpid(pid[i], NULL, 0);
    }

    printf("%d exit rc %d\n", getpid(), exit_udm_cache());
    return 0;
}