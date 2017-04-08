#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define err_exit(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

int error;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void * thr_fn1(void * arg) {
    error = pthread_mutex_lock(&mutex1);
    if(error) {
        err_exit(error, "Thread1 can't lock mutex1");
    }
    printf("Thread1 lock mutex1\n");
    int i;
    for(i = 0 ; i < 0xFFFF ; i++);
    error = pthread_mutex_lock(&mutex2);
    if(error) {
        err_exit(error, "Thread1 can't lock mutex2");
    }
    printf("Thread1 lock mutex2\n");
    pthread_exit((void *)2);
}

void * thr_fn2(void * arg) {
    error = pthread_mutex_lock(&mutex2);
    if(error) {
        err_exit(error, "Thread2 can't lock mutex2");
    }
    printf("Thread2 lock mutex2\n");
    int i;
    for(i = 0 ; i < 0xFFFF ; i++);
    error = pthread_mutex_lock(&mutex1);
    if(error) {
        err_exit(error, "Thread2 can't lock mutex1");
    }
    printf("Thread2 lock mutex1\n");
    pthread_exit((void *)2);
}
        
int main() {
    pthread_t tid1, tid2;
    void *tret;
    error = pthread_create(&tid1, NULL, thr_fn1, (void *)1);
    if (error)
        err_exit(error, "can’t create thread 1");
    error = pthread_create(&tid2, NULL, thr_fn2, (void *)1);
    if (error)
        err_exit(error, "can’t create thread 2");
    error = pthread_join(tid1, &tret);
    if (error)
        err_exit(error, "can’t join with thread 1");
    printf("thread 1 exit code %ld\n", (long)tret);
    error = pthread_join(tid2, &tret);
    if (error)
        err_exit(error, "can’t join with thread 2");
    printf("thread 2 exit code %ld\n", (long)tret);
    exit(EXIT_SUCCESS);
}