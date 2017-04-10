#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define err_exit(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

struct msg {
    struct msg *m_next;
    char * msg;
    /* ... more stuff here ... */
};

struct msg *workq;
pthread_cond_t qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

void print_all_q() {
    struct msg * mp;
    for(mp = workq ; mp != NULL ; mp = mp->m_next) {
        printf("%s ", mp->msg);
    }
    printf("\n");
}

void process_msg(void) {
    struct msg *mp;
    int error;
    for (;;) {
        pthread_mutex_lock(&qlock);
        while (workq == NULL) {
            error = pthread_cond_wait(&qready, &qlock);
            if(error) {
                perror("Can't wait cond");
            }
        }
        mp = workq;
        printf("- dequeue %s\n", workq->msg);
        workq = mp->m_next;
        //print_all_q();
        pthread_mutex_unlock(&qlock);
        /* now process the message mp */
    }
    pthread_exit((void *)2);
}

void enqueue_msg(struct msg *mp) {
    pthread_mutex_lock(&qlock);
    mp->m_next = workq;
    workq = mp;
    printf("+ enqueue %s\n", mp->msg);
    pthread_mutex_unlock(&qlock);
    pthread_cond_signal(&qready);
}

void loop_enqueue_msg(void) {
    int i;
    for(i = 1 ; i <= 50 ; i++) {
        struct msg * mp = malloc(sizeof(struct msg));
        char * buff = malloc(sizeof(char) * 4);
        sprintf(buff, "a%d", i);
        mp->msg = buff;
        //printf("-- %s %x %x\n", mp->msg, mp, buff);
        enqueue_msg(mp);
    }
    pthread_exit((void *)2);
}

int main() {
    int error;
    pthread_t tid1, tid2;
    void *tret;
    error = pthread_create(&tid1, NULL, (void *)process_msg, NULL);
    if (error)
        err_exit(error, "can’t create thread 1");
    error = pthread_create(&tid2, NULL, (void *)loop_enqueue_msg, NULL);
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
