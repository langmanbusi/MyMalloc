#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#include "MyM.h"

void *p1, *p2, *p3, *p4, *p5, *p6;

void *func() {
	printf("Inside thread 1\n");
	void *p = MyMalloc(60);
	void *p8 = MyMalloc(800);
	void *p9 = MyMalloc(900);
	void *p10 = MyMalloc(1000);
	void *p11 = MyMalloc(1100);
	Myfree(p8);
	printf("-----Thread 1 Done----\n");
	return NULL;
}

void *func1() {
	printf("Inside thread 2\n");
	void *p1 = MyMalloc(100);
	void *p2 = MyMalloc(200);
	void *p3 = MyMalloc(300);
	void *p4 = MyMalloc(400);
	void *p5 = MyMalloc(500);
	void *p6 = MyMalloc(600);
	Myfree(p1);
	Myfree(p2);
	Myfree(p5);
	void *p7 = MyMalloc(200);
	/*
	
	list_free();
	list_used();
	//Myfree(p1);
	//Myfree(p2);
	//void *p31 = MyMalloc(100);
	//Myfree(p5);
	//void *p32 = MyMalloc(200);
	//Myfree(p3);*/
	printf("-----Thread 2 Done----\n");
	return NULL;
}

void *func2() {
	void *p7 = MyMalloc(500);
	return NULL;
}

int main() {
	pthread_t t1, t2, t3;
	pthread_create(&t1, NULL, func,  NULL);
	pthread_create(&t2, NULL, func1, NULL);
	//pthread_create(&t3, NULL, func2,  NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	//pthread_join(t3, NULL);
	list_free(1);list_used(1);
	list_free(2);list_used(2);
	return 0;
}
