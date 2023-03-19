#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096 // 缓冲区大小为4KB
#define FILE_SIZE 1024*1024*1024 // 文件大小为1GB

int main()
{
    int fd;
    char *buffer;
    off_t offset;
    ssize_t bytes_read;
    
    // 打开文件，使用O_DIRECT标志打开
    fd = open("testfile", O_RDONLY | O_DIRECT);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 分配内存作为读取缓冲区
    if (posix_memalign(&buffer, BUFFER_SIZE, BUFFER_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    // 随机访问文件并读取数据
    for (offset = 0; offset < FILE_SIZE; offset += BUFFER_SIZE) {
        bytes_read = pread(fd, buffer, BUFFER_SIZE, offset);
        if (bytes_read == -1) {
            perror("pread");
            exit(EXIT_FAILURE);
        }
        printf("Read %zd bytes at offset %ld\n", bytes_read, (long) offset);
    }

    // 关闭文件并释放内存
    close(fd);
    free(buffer);

    return 0;
}