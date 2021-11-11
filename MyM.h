#pragma once
#ifndef __MYM_H__
#define __MYM_H__

#include <pthread.h>

// 8字节对齐的块
typedef struct _Header
{
	char *next;		// 下一块地址
	char *prev;		// 前一块地址
	size_t size;	// 大小
	unsigned int free = 1；// free标志
} __attribute__ ((aligned(8))) Header;

// 线程表块
typedef struct _ThreadTable
{
	long long id;
	char *used;
	char *free;
} ThreadTable;

// 内存管理函数声明
void * MyMalloc(size_t);
void * calloc(size_t, size_t);
void * realloc(void *, size_t);
void Myfree(void *);
void list_free(int);
void list_used(int);

#endif 
