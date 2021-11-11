// 头文件
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "MyM.h"

// 创建未使用和已使用列表里的头地址为空
static char *_head_free = NULL;
static char *_head_used = NULL;

// 全局线程表
static ThreadTable *_thread_tbl = NULL;

// 设置一个全局锁，用于分配共享内存
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

// 把一个块放进未使用列表或已使用列表里
static void add_block(char *block, char **list)
{
	Header *h_ptr;
	Header *blk;

	// 当列表为空时当前块就是首地址
	if (!*list) {
		*list = block;
		blk = (Header *)block;
		blk->prev = NULL;
		return;
	}

	// 否则放在末尾
	h_ptr = (Header *)*list;
	while (h_ptr->next)
		h_ptr = (Header *)h_ptr->next;

	blk = (Header *)block;
	blk->prev = (char *)h_ptr;
	h_ptr->next = block;
}

// 把一个块从未使用列表或已使用列表去除
static int remove_block(char *block, char **list)
{
	Header *h_ptr;
	Header *blk;

	// 列表为空时不操作返回1
	if (!*list || !block)
		return 1;

	h_ptr = (Header *)*list;

	// 当前块是第一个块时
	if (*list == block) {
		if (!h_ptr->next)
			*list = NULL;
		else
			*list = h_ptr->next;
		return 0;
	}

	// 删除块
	while (h_ptr->next != block) {

		if (!h_ptr->next)
			return 1;

		h_ptr = (Header *)h_ptr->next;
	}

	h_ptr->next = ((Header *)h_ptr->next)->next;
	((Header *)h_ptr->next)->prev = (char *)h_ptr;

	return 0;
}

// 检查p是否是8字节对齐，没对齐的话返回多余的字节数。方便后面分块函数
static inline int is_aligned(const void *p, size_t *offset)
{
	*offset = (size_t)((size_t *)p) & 7;
	return !(*offset);
}

// 把一个块分成两个
static void * split(char *block, size_t bytes, char *free)
{
	char *block2;
	char *block_end;
	size_t block_size;
	size_t block2_size;
	Header *h_ptr;
	Header b2header;
	size_t offset;

	h_ptr = (Header *)block;

	// 计算当前block大小
	block_end = block + sizeof(Header) + h_ptr->size;
	block_size = block_end - block;

	// 计算第二块block2应该的大小
	block2 = block + sizeof(Header) + bytes;
	if (!is_aligned(block2, &offset))
		block2 += 8 - offset;

	block2_size = block_end - block2;

	// 要是剩的block太小了就算了
	if (block2_size <= sizeof(Header))
		return block;

	// 接下来分解块
	// 把block从free列表里去除
	remove_block(block, &free);

	// 为block和block2创建新的控制块
	h_ptr->next = NULL;
	h_ptr->prev = NULL;
	h_ptr->size = block_size - block2_size - sizeof(Header);

	b2header.next = NULL;
	b2header.prev = NULL;
	b2header.size = block2_size - sizeof(Header);

	memcpy(block2, &b2header, sizeof(Header));

	// 把这两个分开的块还放进free列表里
	add_block(block, &free);
	add_block(block2, &free);

	return block;
}

// 找到一个块，有bytes的大小，返回块的首地址
static void * find_block(size_t bytes, char *free)
{
	Header *h_ptr;
	size_t numbytes;
	size_t pagesize;
	unsigned char *mem;

	// 检查所有的块，如果有合适的，就分解一下，就在这一步产生了碎片
	h_ptr = (Header *)free;
	while (h_ptr) {
		if (h_ptr->size >= bytes){
			return split((char *)h_ptr, bytes, free);
			//printf("splitfinish");
		}
		h_ptr = (Header *)h_ptr->next;
	}

	// 如果没有就创建一个
	pagesize = sysconf(_SC_PAGESIZE);
	numbytes = (bytes + sizeof(Header) + pagesize - 1) & ~(pagesize - 1);

	// 用mmap创建新的空间
	mem = (unsigned char *)mmap(NULL, numbytes, PROT_READ | PROT_WRITE,
								MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (mem == MAP_FAILED) {
		return NULL;
	}

	// 设置控制块
	h_ptr = (Header *)mem;
	h_ptr->next = NULL;
	// h_ptr->prev = NULL;
	h_ptr->size = numbytes - sizeof(Header);

	// 把这个块放进free列表，其实放进了整页
	add_block((char *)h_ptr, &free);

	// 分解出需要的块，返回首地址
	return (Header *)split((char *)h_ptr, bytes, free);
}

// 输出一下free列表
void list_free(int j)
{
	_head_free = _thread_tbl[j].free;
	long unsigned int ID = _thread_tbl[j].id;
	Header *h_ptr = (Header *)_head_free;
	size_t size;
	Header *location;
	char *next;
	char *prev;

	printf("thread id: %lu\n", ID);
	printf("Free:\n");
	while (h_ptr) {
		location = h_ptr;
		next = h_ptr->next;
		size = h_ptr->size;
		prev = h_ptr->prev;
		printf("\tlocation: %p\n\tnext: %p\n\tprev: %p\n\tsize: %d\n\n", location,
			next, prev, (int)size);

		h_ptr = (Header *)h_ptr->next;
	}
}

// 输出一下used列表
void list_used(int j)
{
	_head_used = _thread_tbl[j].used;
	Header *h_ptr = (Header *)_head_used;
	size_t size;
	Header *location;
	char *next;
	char *prev;

	printf("Used:\n");
	while (h_ptr) {
		location = h_ptr;
		next = h_ptr->next;
		size = h_ptr->size;
		prev = h_ptr->prev;
		printf("\tlocation: %p\n\tnext: %p\n\tprev: %p\n\tsize: %d\n\n", location,
			next, prev, (int)size);

		h_ptr = (Header *)h_ptr->next;
	}
}

// 自定义主干函数，分配bytes空间，返回首地址
void * MyMalloc(size_t bytes)
{
	Header *h_ptr;// 控制块地址
	unsigned int i;// 局部变量i
	size_t num = sysconf(_SC_PAGESIZE) / sizeof(ThreadTable);// 线程数

	// 线程表已经创建
	if (!_thread_tbl) {
		if (pthread_mutex_lock(&_mutex)) {
			return NULL;
		}

		// 检查线程是否在等待锁
		if (!_thread_tbl) {
			_thread_tbl = (ThreadTable *)mmap(NULL, sysconf(_SC_PAGESIZE),
											  PROT_READ | PROT_WRITE,
											  MAP_PRIVATE | MAP_ANONYMOUS,
											  -1, 0);
			
			// 分配内存失败的话解开互斥锁
			if (_thread_tbl == MAP_FAILED) {
				pthread_mutex_unlock(&_mutex);
				return NULL;
			}

			// 初始化free和used表的值
			for (i = 1; i < num; ++i) {
				_thread_tbl[i].id = -1;
				_thread_tbl[i].free = NULL;
				_thread_tbl[i].used = NULL;
			}

			// 最先调用该函数的线程ID给第0块
			_thread_tbl[0].id = pthread_self();
		}
		// 解锁
		pthread_mutex_unlock(&_mutex);
	}

	// 标记线程，每次有一个新的线程调用该函数，就加一个进来
	for (i = 0; i < num; ++i) {

		// 如果就是当前线程再次调用就继续
		if (_thread_tbl[i].id == pthread_self()) {
			break;
		} 

		// 如果不是，就判断是否是空闲，是的话上锁
		else if (_thread_tbl[i].id == -1) {
			if (pthread_mutex_lock(&_mutex)) {
				return NULL;
			}

			// 把另外调用函数的线程ID加进来
			_thread_tbl[i].id = pthread_self();
			// 解锁
			pthread_mutex_unlock(&_mutex);
			break;
		}
	}

	// 如果线程太多把线程空间用完了，就返回空不分配内存
	if (i == num) {
		return NULL;
	}

	// 得到合适块的首地址
	h_ptr = find_block(bytes, _thread_tbl[i].free);
	if (!h_ptr) {
		return NULL;
	}

	// 把该块从未使用列表中去除
	remove_block((char *)h_ptr, &_thread_tbl[i].free);	

	// 放进已使用列表中
	h_ptr->next = NULL;
	// h_ptr->free = 0;
	add_block((char *)h_ptr, &_thread_tbl[i].used);

	// 把对应的free和used列表首地址给对应变量
	//_head_free = _thread_tbl[2].free;
	//_head_used = _thread_tbl[2].used;

	return ((char *)h_ptr) + sizeof(Header);
}

// 重新分配特定大小的块
void * calloc(size_t bytes, size_t value)
{
	char *ptr;

	ptr = (char *)MyMalloc(bytes);
	if (!ptr) {
		return NULL;
	}

	memset(ptr, 0, bytes);

	return ptr;
}

// 将现有的块更新大小
void * realloc(void *ptr, size_t bytes)
{
	char *new_mem;
	Header *h_ptr;

	h_ptr = (Header *)(ptr - sizeof(Header));
	new_mem = (char *)MyMalloc(bytes);
	if (!new_mem) {
		return NULL;
	}

	memcpy(new_mem, ptr, bytes);

	Myfree(ptr);

	return new_mem;
}

// 合并相邻的free块
void coalesce(char *ptr, char **list) 
{
    Header *current_header = (Header *)ptr;
	Header *prv = (Header *)current_header->prev;
	size_t size;

	printf("11\n");

	// 如果有下一块且为free
    if (prv) {
		size = prv->size;
		printf("合并\n");

		// 把下一块从list里删除
		remove_block((char *)prv, list);

		// 把当前块的大小更新
		current_header->size += size;
		current_header->prev = NULL;
	}
	
}

// 释放内存
void Myfree(void *ptr)
{
	Header *h_ptr;
	size_t num = sysconf(_SC_PAGESIZE) / sizeof(ThreadTable);
	unsigned int i;

	if (!ptr) {
		return;
	}

	h_ptr = (Header *)(ptr - sizeof(Header));

	// 标记线程，每次有一个新的线程调用该函数，就加一个进来
	for (i = 0; i < num; ++i) {

		// 如果就是当前线程再次调用就继续
		if (_thread_tbl[i].id == pthread_self()) {
			break;
		} 

		// 如果不是，就判断是否是空闲，是的话上锁
		else if (_thread_tbl[i].id == -1) {
			if (pthread_mutex_lock(&_mutex)) {
				return;
			}

			// 把另外调用函数的线程ID加进来
			_thread_tbl[i].id = pthread_self();
			// 解锁
			pthread_mutex_unlock(&_mutex);
			break;
		}
	}

	if (i == num) {
		return;
	}

	// 把当前块从used列表中去除
	remove_block((char *)h_ptr, &_thread_tbl[i].used);

	h_ptr->next = NULL;

	// 加入到free列表中
	add_block((char *)h_ptr, &_thread_tbl[i].free);

	// 如果有空闲的块就合并
	coalesce((char *)h_ptr, &_thread_tbl[i].free);

}
