#ifndef PIO_H
#define PIO_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: 對 page io的建立與操作，以便後續使用submit_pio向udm-cache提交
 */

#include "config.h"

/* pio is a linked list, it should be aligned to cache block size (32KB) */
struct pio{
	char* full_path_name;	
	unsigned page_index;	/* file offset = page_index * page_size */
	operate operation;	/* READ, WRITE */
	void* buffer;		/* user memory space */
	unsigned pio_cnt;	/* pio nodes count */
	struct pio* next;
};

/* in pio_api.c */
struct pio* create_pio(char* full_path_name, unsigned page_index, operate operation, void* buffer, unsigned pio_cnt);
void append_pio(struct pio* head, void* buffer);
void free_pio(struct pio* head);
void print_pio(struct pio* head);

#endif
