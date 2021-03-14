#ifndef MUTEX_H__
#define MUTEX_H__

#include <pthread.h>

int minit (pthread_mutex_t * m);
int mlock (pthread_mutex_t * m);
int munlock (pthread_mutex_t * m);
void mdestroy (pthread_mutex_t * m);

#endif /* MUTEX_H__ */