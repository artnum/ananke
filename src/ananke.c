#include "ananke.h"

/* must be locked before */
void ananke_error (Message ** stack, const char * message) {
    Message * new = NULL;

    new = msg_new();
    if (new == NULL) { return; }
    if (!msg_printf(new, "{\"operation\": \"error\", \"message\": \"%s\"}", message)) { msg_free(new); return; }
    if(!msg_stack_push(stack, new)) {
        msg_free(new);
    }
}

int ananke_operation (Pair * root, Session * session) {
    char * opname = NULL;
    int i = 0;
    Type vtype;
    void * value;
    size_t vlen;
    int opid = -1;
    Message * msg = NULL;
    if (root == NULL) { return -1; }

    while (OperationMap[i].operation != ANANKE_NOP) {
        if (pair_get_value_at(root, "operation", &vtype, &value, &vlen)) {
            if (vtype != STRING) { i++; continue; }
            if (vlen != OperationMap[i].oplen - 1) { i++; continue; }
            if (strncmp((char *)value, OperationMap[i].opstr, vlen) != 0) { i++; continue; }
            opid = i;
            break;
        }
        i++;
    }

    if (opid < 0) {
        if (!mlock(&(session->mout))) { return; }
        ananke_error(&(session->outstack), "Operation unknown");
        munlock(&(session->mout));
        return 1;
    }
    switch (OperationMap[opid].operation) {
        case ANANKE_CLOSE:
            if (!mlock(&(session->mutex))) { return -1; }
            session->end = 1;
            munlock(&(session->mutex));
            return 0;
        case ANANKE_PING:
            if(!mlock(&(session->mout))) { return -1; }
            ananke_error(&(session->outstack), "It's my job to ping");
            munlock(&(session->mout));
            return 1;
        case ANANKE_PONG:
            if (!pair_get_value_at(root, "count", &vtype, &value, &vlen)) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "Missing argument for pong");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != INTEGER) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "Wrong argument type, count must be int");
                munlock(&(session->mout));
                return 1;
            }
            if (!mlock(&(session->mutex))) { return -1; }
            session->pongReceived = *((int *)value);
            munlock(&(session->mutex));
            return 0;
        case ANANKE_LOCK_RESOURCE:
            if (!pair_get_value_at(root, "resource", &vtype, &value, &vlen)) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "Missing argument for lock-resource");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != STRING) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "Resource argument must be string");
                munlock(&(session->mout));
                return 1;
            }
            i = lock(session->lockCtx, (char *)value, vlen, NULL);
            msg = msg_new();
            msg_printf(msg, "{\"operation\": \"lock\", \"lockid\": %d}", i);
            if (!mlock(&(session->mout))) { unlock(session->lockCtx, i); return -1; }
            msg_stack_push(&(session->outstack), msg);
            munlock(&(session->mout));
            return 1;
        case ANANKE_UNLOCK_RESOURCE:
            if (!pair_get_value_at(root, "lockid", &vtype, &value, &vlen)) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "Missing argument for unlock-resource\n");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != INTEGER) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), "lockid must be integer\n");
                munlock(&(session->mout));
                return 1;
            }
            if (!unlock(session->lockCtx, *((int *)value))) {
                lwsl_err("Cannot unlock resource\n");
            }
            msg = msg_new();
            msg_printf(msg, "{\"operation\": \"unlock\", \"done\": true}");
            if (!mlock(&(session->mout))) { unlock(session->lockCtx, i); return -1; }
            msg_stack_push(&(session->outstack), msg);
            munlock(&(session->mout));
            return 1;
    }
}