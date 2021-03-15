#ifndef MUTEX_H__
#define MUTEX_H__

#include <pthread.h>

#define AK_MUTEX_USLEEP 10
#define AK_MUTEX_MAXTRY 5

typedef pthread_mutex_t AKMutex;
typedef pthread_cond_t AKCond;

int minit (AKMutex * m);
int mlock (AKMutex * m);
int munlock (AKMutex * m);
void mdestroy (AKMutex * m);
void mcondsignal(AKCond * c);
void mcondwait (AKCond * c, AKMutex * m);
void mcondinit (AKCond * c);
void mconddestroy(AKCond * c);

#endif /* MUTEX_H__ */