#include "include/mutex.h"
#include <unistd.h>
#include <errno.h>

int minit (AKMutex * m) {
    if (m == NULL) { return 0; }
    pthread_mutex_init(m, NULL);
    return 1;
}

int mlock (AKMutex * m) {
    int mret = 0;
    int trycount = 0;
    if (m == NULL) { return 0; }
    do {
        mret = 0;
        mret = pthread_mutex_trylock(m);
        trycount++;
        if (mret == EBUSY) { usleep(AK_MUTEX_USLEEP); } 
    } while (mret != 0 && trycount < AK_MUTEX_MAXTRY);
    if (mret != 0) { 
        return 0; 
    }
    return 1;
}

int munlock (AKMutex * m) {
    int mret = 0;
    if (m == NULL) { return 0; }
    mret = pthread_mutex_unlock(m);
    if (mret != 0) { return 0; }
    return 1;
}

/* destroy a locked mutex */
void mdestroy (AKMutex * m) {
    if (m == NULL) { return; }
    pthread_mutex_unlock(m);
    pthread_mutex_destroy(m);
}

void mcondinit (AKCond * c) {
    if (c == NULL) { return; }
    pthread_cond_init(c, NULL);
    return;
}

void mcondwait (AKCond * c, AKMutex * m) {
    if (c == NULL) { return; }
    if (m == NULL) { return; }
    pthread_cond_wait(c, m);
}

void mcondsignal(AKCond * c) {
    if (c == NULL) { return ; }
    pthread_cond_signal(c);
}

void mconddestroy(AKCond * c) {
    if (c == NULL) { return; }
    pthread_cond_destroy(c);
}