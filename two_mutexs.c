#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define err_exit(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define NHASH 29
#define HASH(id) (((unsigned long)id)%NHASH)
struct foo *fh[NHASH];
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;
struct foo {
    int f_count;
    pthread_mutex_t f_lock;
    int f_id;
    struct foo *f_next; /* protected by hashlock */
    /* ... more stuff here ... */
};

struct foo * foo_alloc(int id) { /* allocate the object */
    struct foo *fp;
    int idx;
    if ((fp = malloc(sizeof(struct foo))) != NULL) {
        fp->f_count = 1;
        fp->f_id = id;
        if (pthread_mutex_init(&fp->f_lock, NULL) != 0) {
            free(fp);
            return(NULL);
        }
        idx = HASH(id);
        pthread_mutex_lock(&hashlock);
        fp->f_next = fh[idx];
        fh[idx] = fp;
        pthread_mutex_lock(&fp->f_lock);
        pthread_mutex_unlock(&hashlock);
        /* ... continue initialization ... */
        pthread_mutex_unlock(&fp->f_lock);
    }
    return(fp);
}

void foo_hold(struct foo *fp) { /* add a reference to the object */
    pthread_mutex_lock(&fp->f_lock);
    fp->f_count++;
    pthread_mutex_unlock(&fp->f_lock);
}

struct foo * foo_find(int id) { /* find an existing object */
    struct foo *fp;
    pthread_mutex_lock(&hashlock);
    for (fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
        if (fp->f_id == id) {
            foo_hold(fp);
            break;
        }
    }
    pthread_mutex_unlock(&hashlock);
    return(fp);
}

void foo_rele(struct foo *fp) { /* release a reference to the object */
    struct foo *tfp;
    int idx;
    pthread_mutex_lock(&fp->f_lock);
    if (fp->f_count == 1) { /* last reference */
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_lock(&hashlock);
        pthread_mutex_lock(&fp->f_lock);
        /* need to recheck the condition */
        if (fp->f_count != 1) {
            fp->f_count--;
            pthread_mutex_unlock(&fp->f_lock);
            pthread_mutex_unlock(&hashlock);
            return;
        }
        /* remove from list */
        idx = HASH(fp->f_id);
        tfp = fh[idx];
        if (tfp == fp) {
            fh[idx] = fp->f_next;
        } else {
            while (tfp->f_next != fp)
                tfp = tfp->f_next;
            tfp->f_next = fp->f_next;
        }
        pthread_mutex_unlock(&hashlock);
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
    } else {
        fp->f_count--;
        pthread_mutex_unlock(&fp->f_lock);
    }
}

void id_entry(int x) {
    int i;
    for(i = 0 ; i < 20 ; i++) {
        foo_alloc(x + i);
    }
    pthread_exit((void *)2);
}

void print_all_foo_entry() {
    int i;
    for(i = 0 ; i < NHASH ; i++) {
        struct foo * tmp;
        if(fh[i] != NULL)
            printf("fh[%2d]: ", i);
        for(tmp = fh[i] ; tmp != NULL ; tmp = tmp->f_next ) {
            printf("%d", tmp->f_id);
            if(tmp->f_next != NULL) {
                printf(" -> ");
            }
        }
        if(fh[i] != NULL)
            printf("\n");
    }
}



int main() {
    int error;
    pthread_t tid1, tid2, tid3;
    void *tret;

    error = pthread_create(&tid1, NULL, id_entry, 0);
    if (error)
        err_exit(error, "can’t create thread 1");

    error = pthread_create(&tid2, NULL, id_entry, 29);
    if (error)
        err_exit(error, "can’t create thread 2");

    error = pthread_create(&tid3, NULL, id_entry, 35);
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

    error = pthread_join(tid3, &tret);
    if (error)
        err_exit(error, "can’t join with thread 3");
    printf("thread 3: exit code %ld\n", (long)tret);

    print_all_foo_entry();
    exit(EXIT_SUCCESS);
}