#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define err_exit(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define get_time_to_buf(tout) \
        clock_gettime(CLOCK_REALTIME, &tout);   \
        tmp = localtime(&tout.tv_sec);          \
        strftime(buf, sizeof(buf), "%r", tmp);  \

int error;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void * thr_fn1(void * arg) {
    struct timespec tout;
    struct tm *tmp;
    char buf[64];
    get_time_to_buf(tout);
    //printf("current time is %s\n", buf);
    tout.tv_sec += 5; /* 5 seconds from now */
    /* caution: this could lead to deadlock */
    //error = pthread_mutex_lock(&mutex1);
    error = pthread_mutex_timedlock(&mutex1, &tout);
    get_time_to_buf(tout);
    //printf("the time is now %s\n", buf);
    if (error) {
        printf("Thread 1: can’t lock mutex1: %s\n", strerror(error));
    }
    else {
        printf("Thread 1: mutex1 locked!\n");
    }
    int i;
    for(i = 0 ; i < 0xFFFF ; i++);
    get_time_to_buf(tout);
    //printf("current time is %s\n", buf);
    tout.tv_sec += 5; /* 5 seconds from now */
    /* caution: this could lead to deadlock */
    error = pthread_mutex_timedlock(&mutex2, &tout);
    get_time_to_buf(tout);
    //printf("the time is now %s\n", buf);
    if (error) {
        printf("Thread 1: can’t lock mutex2: %s\n", strerror(error));
    }
    else {
        printf("Thread 1: mutex2 locked!\n");
    }
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    pthread_exit((void *)2);
}

void * thr_fn2(void * arg) {
    struct timespec tout;
    struct tm *tmp;
    char buf[64];
    get_time_to_buf(tout);
    //printf("current time is %s\n", buf);
    tout.tv_sec += 5; /* 5 seconds from now */
    /* caution: this could lead to deadlock */
    //error = pthread_mutex_lock(&mutex2);
    error = pthread_mutex_timedlock(&mutex2, &tout);
    get_time_to_buf(tout);
    //printf("the time is now %s\n", buf);
    if (error) {
        printf("Thread 2: can’t lock mutex2: %s\n", strerror(error));
    }
    else {
        printf("Thread 2: mutex2 locked!\n");
    }
    int i;
    for(i = 0 ; i < 0xFFFF ; i++);
    get_time_to_buf(tout);
    //printf("current time is %s\n", buf);
    tout.tv_sec += 10; /* 10 seconds from now */
    /* caution: this could lead to deadlock */
    error = pthread_mutex_timedlock(&mutex1, &tout);
    get_time_to_buf(tout);
    //printf("the time is now %s\n", buf);
    if (error) {
        printf("Thread 2: can’t lock mutex1: %s\n", strerror(error));
    }
    else {
        printf("Thread 2: mutex1 locked!\n");
    }
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
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
    printf("thread 1: exit code %ld\n", (long)tret);
    error = pthread_join(tid2, &tret);
    if (error)
        err_exit(error, "can’t join with thread 2");
    printf("thread 2: exit code %ld\n", (long)tret);
    exit(EXIT_SUCCESS);
}