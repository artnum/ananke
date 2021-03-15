#include "lock.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void lock_free(Lock * l) {
    int canFree = 0;
    if (l == NULL) { return; }
    pthread_mutex_lock(&(l->mutex));
    l->on--;
    canFree = l->on;
    pthread_mutex_unlock(&(l->mutex));
    if (canFree < 0) {
        if (l != NULL) {
            if (l->resource != NULL) { free(l->resource); }
            free(l);
        }
    }
}

Lock * lock_create(char * resource, size_t rlen, void * host) {
    Lock * new = NULL;

    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }
    new->resource = calloc(rlen + 1, sizeof(*resource));
    if (new->resource == NULL) { lock_free(new); return NULL; }

    if (time(&(new->time)) == -1) { lock_free(new); return NULL; }
    memcpy(new->resource, resource, rlen);
    new->rlen = rlen;
    new->host = host;
    new->on = 0;
    pthread_mutex_init(&(new->mutex), NULL);

    return new;
}

int lock (LockContext * ctx, char * resource, size_t rlen, void * host) {
    Lock * new = NULL;
    Lock * old = NULL;
    int getTry = 0;
    int lockId = 0;

    /* get lock, if none can lock else lock is refused */
    old = lock_get(ctx, resource, rlen, &getTry);
    if (old) { lock_free(old); return 0; }
    if (getTry >= LOCK_MAX_TRY) { return 0; }

    new = lock_create(resource, rlen, host);
    if (new == NULL) { return 0; }

    lockId = lock_insert(ctx, new);
    if (!lockId) { lock_free(new); return 0; }
    return lockId;
}

int unlock (LockContext * ctx, int lockId) {
    Lock * old = NULL;

    old = lock_remove(ctx, lockId);
    if (old) {
        lock_free(old);
        return 1;
    }
    return 0;
}

Lock * lock_get (LockContext * ctx, char * resource, size_t rlen, int * count) {
    int i = 0;
    Lock * found = NULL;
    int skipped = 0;
    Lock * current = NULL;

    pthread_mutex_lock(&(ctx->mutex));
    for (i = 0; i < ctx->size; i++) {
        if (*(ctx->origin + i) != NULL) {
            current = *(ctx->origin + i);
            if (pthread_mutex_trylock(&(current->mutex)) != EBUSY) {
                if (rlen != current->rlen) { pthread_mutex_unlock(&(current->mutex)); continue; }
                if (strncmp(resource, current->resource, rlen) == 0) {
                    current->on++;
                    found = current;
                } 
                pthread_mutex_unlock(&(current->mutex));
                if (found) { break; }
            } else {
                skipped++;
            }
        }
    }
    pthread_mutex_unlock(&(ctx->mutex));

    /* not found but some lock skipped, wait and retry x10 */
    if (!found && skipped > 0) {
        if (*count < LOCK_MAX_TRY) {
            usleep(50);
            (*count)++;
            return lock_get(ctx, resource, rlen, count);
        }
    }

    return found;
}

Lock * lock_remove (LockContext * ctx, int lockId) {
    Lock * old = NULL;

    pthread_mutex_lock(&(ctx->mutex));
    if (lockId - 1 < ctx->size) {
        old = *(ctx->origin + lockId - 1);
        *(ctx->origin + lockId - 1) = NULL;
    }
    pthread_mutex_unlock(&(ctx->mutex));

    return old;
}

int lock_insert (LockContext * ctx, Lock * new) {
    int i = 0;
    int lockId = 0;

    pthread_mutex_lock(&(ctx->mutex));
    for (i = 0; i < ctx->size; i++) {
        if (*(ctx->origin + i) == NULL) {
            lockId = i + 1;
            *(ctx->origin + i) = new;
            new->id = lockId;
            break;
        }
    }
    pthread_mutex_unlock(&(ctx->mutex));

    if (lockId != 0) { return new->id; }
    if(!lock_ctx_grow(ctx)) { return 0; }
    return lock_insert(ctx, new);
}

int lock_ctx_grow(LockContext * ctx) {
    int i = 0;
    int newSize = 0;
    void * resized = NULL;

    pthread_mutex_lock(&(ctx->mutex));
    newSize = ctx->size + LOCK_CTX_BLOCK_SIZE;
    resized = realloc(ctx->origin, sizeof(*(ctx->origin)) * newSize);
    if (resized != NULL) {
        ctx->origin = (Lock **)resized;
        for (i = ctx->size; i < newSize; i++) {
            *(ctx->origin + i) = NULL;
        }
        ctx->size = newSize;
    }
    pthread_mutex_unlock(&(ctx->mutex));

    if (resized == NULL) { return 0; }
    return 1;
}

LockContext * lock_ctx_init(void) {
    LockContext * new = NULL;

    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }

    new->size = 0;
    pthread_mutex_init(&(new->mutex), NULL);
    if (!lock_ctx_grow(new)) { lock_ctx_destroy(new); return NULL; }

    return new;
}

void lock_ctx_destroy(LockContext * ctx) {
    int i = 0;

    pthread_mutex_lock(&(ctx->mutex));
    for (i = 0; i < ctx->size; i++) {
        lock_free(*(ctx->origin + i));
    }
    pthread_mutex_unlock(&(ctx->mutex));
    pthread_mutex_destroy(&(ctx->mutex));
    free(ctx->origin);
    free(ctx);
}