#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IO_TIME 1000000
#define PAGE_SIZE 4096

int main(int argc, char *argv[]) {
    int fd;
    void *buffer;
    ssize_t bytes_written;
    off_t offset = 0;

    fd = open("testfile", O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < IO_TIME; i++) {
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, offset);
        offset += PAGE_SIZE;
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);

    return 0;
}