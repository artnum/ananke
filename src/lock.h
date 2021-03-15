#ifndef LOCK_H__
#define LOCK_H__

#include <time.h>
#include <pthread.h>

#define LOCK_CTX_BLOCK_SIZE 100
#define LOCK_MAX_TRY 10

typedef struct _s_lock {
    time_t time;
    size_t rlen;
    char * resource;
    void * host;
    int on;
    int id;
    pthread_mutex_t mutex;
} Lock;

typedef struct _s_lockCtx {
    Lock ** origin;
    int size;
    pthread_mutex_t mutex;
} LockContext;

void lock_free(Lock * l);
Lock * lock_create(char * resource, size_t rlen, void * host);
int lock (LockContext * ctx, char * resource, size_t rlen, void * host);
int unlock (LockContext * ctx, int lockId);
Lock * lock_get (LockContext * ctx, char * resource, size_t rlen, int * count);
Lock * lock_remove (LockContext * ctx, int lockId);
int lock_insert (LockContext * ctx, Lock * new);
int lock_ctx_grow(LockContext * ctx);
LockContext * lock_ctx_init(void);
void lock_ctx_destroy(LockContext * ctx);

#endif /* LOCK_H__ */