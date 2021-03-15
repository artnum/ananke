#ifndef ANANKE_H__
#define ANANKE_H__

#include <stddef.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <stdarg.h>
#include "msg.h"
#include "mutex.h"
#include "prot.h"
#include "lock.h"

#define AK_PING_INTERVAL    5000000 // us between ping

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
    struct _s_anankeErrorMap *errmap;
} Session;

typedef enum _e_anankeOp {
    ANANKE_NOP = -1,
    ANANKE_PING = 0,
    ANANKE_PONG,
    ANANKE_CLOSE,
    ANANKE_LOCK_RESOURCE,
    ANANKE_UNLOCK_RESOURCE,
    ANANKE_SET_LOCALE
} AnankeOp;

typedef enum _e_anankeErrorCode {
    AK_ERR_NONE = -1,
    AK_ERR_SUCCESS = 0,
    AK_ERR_OPERATION_FAILED,
    AK_ERR_UNKNOWN_OP,
    AK_ERR_NOT_ALLOWED,
    AK_ERR_MISSING_ARGUMENT,
    AK_ERR_WRONG_TYPE

} AnankeErrorCode;

struct _s_anankeOpMap {
    AnankeOp operation;
    const char * opstr;
    const size_t oplen;
};

struct _s_anankeErrorMap {
    AnankeErrorCode code;
    const char * msg;
};

struct _s_anankeErrorMapLang {
    const char * lang;
    const struct _s_anankeErrorMap * errmap;
};

static const struct _s_anankeOpMap OperationMap[] = {
    {ANANKE_PING, "ping", sizeof("ping")},
    {ANANKE_PONG, "pong", sizeof("pong")},
    {ANANKE_CLOSE, "close", sizeof("close")},
    {ANANKE_LOCK_RESOURCE, "lock-resource", sizeof("lock-resource")},
    {ANANKE_UNLOCK_RESOURCE, "unlock-resource", sizeof("unlock-resource")},
    /* error code are sent translated (if available) after that command */
    {ANANKE_SET_LOCALE, "set-locale", sizeof("set-locale")},
    {ANANKE_NOP, NULL}
};

static const struct _s_anankeErrorMap EnErrorMap[] = {
    { AK_ERR_SUCCESS, "Success" },
    { AK_ERR_OPERATION_FAILED, "Operation failed" },
    { AK_ERR_UNKNOWN_OP, "Operation unknown" },
    { AK_ERR_NOT_ALLOWED, "Operation not allowed" },
    { AK_ERR_MISSING_ARGUMENT, "Argument missing for operation" },
    { AK_ERR_WRONG_TYPE, "Argument has the wrong type"},
    { AK_ERR_NONE, NULL }
};

static const struct _s_anankeErrorMap FrErrorMap[] = {
    { AK_ERR_SUCCESS, "Succès" },
    { AK_ERR_OPERATION_FAILED, "Échec de l'opération" },
    { AK_ERR_UNKNOWN_OP, "Opération inconnue" },
    { AK_ERR_NOT_ALLOWED, "Opération non autoriése" },
    { AK_ERR_MISSING_ARGUMENT, "Argument manquant pour l'opération" },
    { AK_ERR_WRONG_TYPE, "Mauvais type pour l'argument" },
    { AK_ERR_NONE, NULL }
};

static const struct _s_anankeErrorMapLang ErrorMapLocale[] = {
    { "en", &EnErrorMap },
    { "fr", &FrErrorMap },
    { NULL, NULL}
};

void ananke_error (Session * session, AnankeErrorCode code, char * details);
void ananke_message (Session * session, char * format, ...);
int ananke_operation (Pair * root, Session * session);
#endif /* ANANKE_H__ */