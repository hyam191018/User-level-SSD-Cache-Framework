#include "pio.h"

struct pio *create_pio(char *full_path_name, int fd, unsigned page_index, operate operation,
                       void *buffer, unsigned pio_cnt) {
    struct pio *new_pio = (struct pio *)malloc(sizeof(struct pio));
    if (!new_pio) {
        return NULL;
    }
    new_pio->full_path_name = full_path_name;
    new_pio->fd = fd;
    new_pio->page_index = page_index;
    new_pio->operation = operation;
    new_pio->buffer = buffer;
    new_pio->pio_cnt = pio_cnt;
    new_pio->next = NULL;
    return new_pio;
}

void append_pio(struct pio *head, void *buffer) {
    struct pio *current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = create_pio(current->full_path_name, current->fd, current->page_index + 1,
                               current->operation, buffer, current->pio_cnt);
}

void free_pio(struct pio *head) {
    if (head == NULL) {
        return;
    }
    free_pio(head->next);
    free(head);
}

void print_pio(struct pio *head) {
    struct pio *current = head;
    while (current != NULL) {
        printf("------------------------------\n");
        printf("full path name = %s\n", current->full_path_name);
        printf("file descriptor = %d\n", current->fd);
        printf("page index = %d\n", current->page_index);
        switch (current->operation) {
            case READ:
                printf("operation = READ\n");
                break;
            case WRITE:
                printf("operation = WRITE\n");
                break;
            default:
                printf("operation = UNKNOWN\n");
                break;
        }
        printf("buf = %p\n", current->buffer);
        printf("pio count = %d\n", current->pio_cnt);
        printf("------------------------------\n");
        current = current->next;
    }
}