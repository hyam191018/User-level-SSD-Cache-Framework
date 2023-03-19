#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 4096 // 缓冲区大小为4KB
#define FILE_SIZE 1024*1024*1024 // 文件大小为1GB

char get_random_uppercase_letter() {
    return 'A' + (rand() % 26);
}

int main()
{
    int fd;
    char *buffer;
    off_t offset;
    ssize_t bytes_written;
    
    // 打开文件，使用O_DIRECT标志打开
    fd = open("testfile", O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 分配内存作为写入缓冲区
    if (posix_memalign(&buffer, BUFFER_SIZE, BUFFER_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    
    // 填充缓冲区，使用随机大写字母
    srand(time(NULL));
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = get_random_uppercase_letter();
    }

    // 随机访问文件并写入数据
    for (offset = 0; offset < FILE_SIZE; offset += BUFFER_SIZE) {
        bytes_written = pwrite(fd, buffer, BUFFER_SIZE, offset);
        if (bytes_written == -1) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
        printf("Wrote %zd bytes at offset %ld\n", bytes_written, (long) offset);
    }

    // 关闭文件并释放内存
    close(fd);
    free(buffer);

    return 0;
}