#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

#include "myalloc.h"

void *p1, *p2, *p3, *p4, *p5, *p6;

void *func() {
	printf("Inside thread 1\n");
	void *p = malloc(60);
	void *p8 = malloc(800);
	void *p9 = malloc(900);
	void *p10 = malloc(1000);
	void *p11 = malloc(1100);
	free(p8);
	printf("-----Thread 1 Done----\n");
	return NULL;
}

void *func1() {
	printf("Inside thread 2\n");
	void *p1 = malloc(100);
	void *p2 = malloc(200);
	void *p3 = malloc(300);
	void *p4 = malloc(400);
	void *p5 = malloc(500);
	void *p6 = malloc(600);
	free(p1);
	free(p2);
	free(p5);
	void *p7 = malloc(200);
	/*
	
	list_free();
	list_used();
	//free(p1);
	//free(p2);
	//void *p31 = malloc(100);
	//free(p5);
	//void *p32 = malloc(200);
	//free(p3);*/
	printf("-----Thread 2 Done----\n");
	return NULL;
}

void *func2() {
	void *p7 = malloc(500);
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
