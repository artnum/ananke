#ifndef MUTEX_H__
#define MUTEX_H__

#include <pthread.h>

#define AK_MUTEX_USLEEP 10
#define AK_MUTEX_MAXTRY 5

int minit (pthread_mutex_t * m);
int mlock (pthread_mutex_t * m);
int munlock (pthread_mutex_t * m);
void mdestroy (pthread_mutex_t * m);

#endif /* MUTEX_H__ */