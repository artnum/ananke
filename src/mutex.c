#include "mutex.h"
#include <unistd.h>
#include <errno.h>

int minit (pthread_mutex_t * m) {
    if (m == NULL) { return 0; }
    pthread_mutex_init(m, NULL);
    return 1;
}

int mlock (pthread_mutex_t * m) {
    int mret = 0;
    int trycount = 0;
    if (m == NULL) { return 0; }
    do {
        mret = 0;
        mret = pthread_mutex_trylock(m);
        trycount++;
        if (mret == EBUSY) { usleep(50); } 
    } while (mret != 0 && trycount < 5);
    if (mret != 0) { 
        return 0; 
    }
    return 1;
}

int munlock (pthread_mutex_t * m) {
    int mret = 0;
    if (m == NULL) { return 0; }
    mret = pthread_mutex_unlock(m);
    if (mret != 0) { return 0; }
    return 1;
}

/* destroy a locked mutex */
void mdestroy (pthread_mutex_t * m) {
    if (m == NULL) { return; }
    pthread_mutex_unlock(m);
    pthread_mutex_destroy(m);
}

void mcondinit (pthread_cond_t * c) {
    if (c == NULL) { return; }
    pthread_cond_init(c, NULL);
    return;
}

void mcondwait (pthread_cond_t * c, pthread_mutex_t * m) {
    if (c == NULL) { return; }
    if (m == NULL) { return; }
    pthread_cond_wait(c, m);
}

void mcondsignal(pthread_cond_t * c) {
    if (c == NULL) { return ; }
    pthread_cond_signal(c);
}