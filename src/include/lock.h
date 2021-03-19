#ifndef LOCK_H__
#define LOCK_H__

#include <time.h>
#include <pthread.h>
#include "mutex.h"

#define LOCK_CTX_BLOCK_SIZE 100
#define LOCK_MAX_TRY 10
#include <uuid.h>

typedef struct _s_lock {
    time_t time;
    size_t rlen;
    char * resource;
    void * host;
    int on;
    uuid_t id;
    AKMutex mutex;
} Lock;

typedef struct _s_lockCtx {
    Lock ** origin;
    int size;
    AKMutex mutex;
} LockContext;

void lock_free(Lock * l);
Lock * lock_create(char * resource, size_t rlen, void * host);
char * lock (LockContext * ctx, char * resource, size_t rlen, void * host);
int unlock (LockContext * ctx, char * lockId);
Lock * lock_get (LockContext * ctx, char * resource, size_t rlen, int * count);
Lock * lock_remove (LockContext * ctx, uuid_t id);
int lock_insert (LockContext * ctx, Lock * new);
int lock_ctx_grow(LockContext * ctx);
LockContext * lock_ctx_init(void);
void lock_ctx_destroy(LockContext * ctx);

#endif /* LOCK_H__ */