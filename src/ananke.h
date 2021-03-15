#ifndef ANANKE_H__
#define ANANKE_H__

#include <stddef.h>
#include <libwebsockets.h>
#include <pthread.h>
#include "msg.h"
#include "mutex.h"
#include "prot.h"
#include "lock.h"

typedef struct _s_session {
    Message * current;
    Message * instack;
    Message * outstack;
    int pingCount;
    int pongReceived;
    int end;

    struct lws *wsi;
    pthread_t userthread;
    pthread_cond_t condition;
    pthread_mutex_t mutex;
    pthread_mutex_t mout;
    pthread_mutex_t min;
    LockContext * lockCtx;
} Session;

typedef enum _e_anankeOp {
    ANANKE_NOP = -1,
    ANANKE_PING = 0,
    ANANKE_PONG,
    ANANKE_CLOSE,
    ANANKE_LOCK_RESOURCE,
    ANANKE_UNLOCK_RESOURCE
} AnankeOp;

struct _s_anankeOpMap {
    AnankeOp operation;
    const char * opstr;
    const size_t oplen;
};

static const struct _s_anankeOpMap OperationMap[] = {
    {ANANKE_PING, "ping", sizeof("ping")},
    {ANANKE_PONG, "pong", sizeof("pong")},
    {ANANKE_CLOSE, "close", sizeof("close")},
    {ANANKE_LOCK_RESOURCE, "lock-resource", sizeof("lock-resource")},
    {ANANKE_UNLOCK_RESOURCE, "unlock-resource", sizeof("unlock-resource")},
    {ANANKE_NOP, NULL}
};

int ananke_operation (Pair * root, Session * session);
#endif /* ANANKE_H__ */