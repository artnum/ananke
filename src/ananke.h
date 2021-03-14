#ifndef ANANKE_H__
#define ANANKE_H__

#include <stddef.h>

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
};

static const struct _s_anankeOpMap OperationMap[] = {
    {ANANKE_PING, "ping"},
    {ANANKE_PONG, "pong"},
    {ANANKE_CLOSE, "close"},
    {ANANKE_LOCK_RESOURCE, "lock-resource"},
    {ANANKE_UNLOCK_RESOURCE, "unlock-resource"},
    {ANANKE_NOP, NULL}
};

#endif /* ANANKE_H__ */