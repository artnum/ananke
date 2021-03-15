#include "ananke.h"

/* must be locked before */
void ananke_error (Message ** stack, AnankeErrorCode code, char * details) {
    Message * new = NULL;
    va_list args;
    int i = 0;

    while (ErrorMap[i].code != AK_ERR_NONE) {
        if (ErrorMap[i].code == code) { break; }
        i++;
    }

    new = msg_new();
    if (new == NULL) { return; }
    if (!msg_printf(new, "{\"operation\": \"error\", \"message\": \"%s\", \"code\": %d, \"details\": \"%s\"}", ErrorMap[i].msg, ErrorMap[i].code, details != NULL ? details : "")) { msg_free(new); return; }
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
            if (vtype != AK_ENC_STRING) { i++; continue; }
            if (vlen != OperationMap[i].oplen - 1) { i++; continue; }
            if (strncmp((char *)value, OperationMap[i].opstr, vlen) != 0) { i++; continue; }
            opid = i;
            break;
        }
        i++;
    }

    if (opid < 0) {
        if (!mlock(&(session->mout))) { return; }
        ananke_error(&(session->outstack), AK_ERR_UNKNOWN_OP, NULL);
        munlock(&(session->mout));
        return 1;
    }
    switch (OperationMap[opid].operation) {
        case ANANKE_CLOSE:
            if (!mlock(&(session->mutex))) { return -1; }
            session->end = 1;
            munlock(&(session->mutex));
            mcondsignal(&(session->condition));
            return 0;
        case ANANKE_PING:
            if(!mlock(&(session->mout))) { return -1; }
            ananke_error(&(session->outstack), AK_ERR_NOT_ALLOWED, NULL);
            munlock(&(session->mout));
            return 1;
        case ANANKE_PONG:
            if (!pair_get_value_at(root, "count", &vtype, &value, &vlen)) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), AK_ERR_MISSING_ARGUMENT, "count");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != AK_ENC_INTEGER) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), AK_ERR_WRONG_TYPE, "count(integer)");
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
                ananke_error(&(session->outstack), AK_ERR_MISSING_ARGUMENT, "resource");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), AK_ERR_WRONG_TYPE, "resource(string)");
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
                ananke_error(&(session->outstack), AK_ERR_MISSING_ARGUMENT, "lockid");
                munlock(&(session->mout));
                return 1;
            }
            if (vtype != AK_ENC_INTEGER) {
                if (!mlock(&(session->mout))) { return -1; }
                ananke_error(&(session->outstack), AK_ERR_WRONG_TYPE, "lockid(integer)");
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