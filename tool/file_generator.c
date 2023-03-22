#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FILE_SIZE 200 * 1024 * 1024  // 檔案大小为200MB
#define PAGE_SIZE 4096

int main(int argc, char *argv[]) {
    int fd;
    void *buffer;
    ssize_t bytes_written;
    off_t offset;

    fd = open("testfile", O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    for (offset = 0; offset < FILE_SIZE; offset += PAGE_SIZE) {
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, offset);
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);

    return 0;
}