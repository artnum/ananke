#include "ananke.h"

/* must be locked before */
void ananke_error (Session * session, AnankeErrorCode code, char * details) {
    Message * msg = NULL;
    int i = 0;
    struct _s_anankeErrorMap *errmap = &EnErrorMap;

    if (mlock(&(session->mutex))) {
        errmap = session->errmap;
        munlock(&(session->mutex));
    }

    while (errmap[i].code != AK_ERR_NONE) {
        if (errmap[i].code == code) { break; }
        i++;
    }

    if ((msg = msg_new(AK_MSG_STRING)) == NULL) { return; }
    if (!mlock(&(session->mutex))) { free(msg); return; }
    if (!msg_printf(
        msg,
        "{\"operation\": \"error\", \"message\": \"%s\", \"code\": %d, \"details\": \"%s\"}",
        errmap[i].msg,
        errmap[i].code, details != NULL ? details : "")) { 
            munlock(&(session->mutex));
            msg_free(msg);
            return; 
        }
    munlock(&(session->mutex));

    msgstack_push(&(session->out), msg, NULL);

    if (mlock(&(session->mutex))) {
        lws_callback_on_writable(session->wsi);
        munlock(&(session->mutex));
    }
}

void ananke_message (Session * session, char * format, ...) {
    va_list args;
    Message * msg = NULL;
    
    va_start(args, format);
    msg = msg_new(AK_MSG_STRING);
    if (msg == NULL) { return; }

    /* session lock happend before msg_printf, session->... might be used as argument */
    if (!mlock(&(session->mutex))) { msg_free(msg); return; }
    if (!msg_vprintf(msg, format, args)) { munlock(&(session->mutex)); msg_free(msg); return; }
    munlock(&(session->mutex));

    msgstack_push(&(session->out), msg, NULL);
    
    if (mlock(&(session->mutex))) {
        lws_callback_on_writable(session->wsi);
        munlock(&(session->mutex));
    }
    va_end(args);
}

int ananke_operation (Pair * root, Session * session) {
    char * opname = NULL;
    int i = 0;
    int success = 0;
    AKType vtype;
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
        ananke_error(session, AK_ERR_UNKNOWN_OP, NULL);
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
            ananke_error(session, AK_ERR_NOT_ALLOWED, NULL);
            return 1;
        case ANANKE_PONG:
            if (!pair_get_value_at(root, "count", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "count");
                return 1;
            }
            if (vtype != AK_ENC_INTEGER) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "count(integer)");
                return 1;
            }
            if (!mlock(&(session->mutex))) { return -1; }
            session->pongReceived = *((int *)value);
            munlock(&(session->mutex));
            return 0;
        case ANANKE_LOCK_RESOURCE:
            if (!pair_get_value_at(root, "resource", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "resource");
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "resource(string)");
                return 1;
            }
            i = lock(session->lockCtx, (char *)value, vlen, NULL);
            ananke_message(session, "{\"operation\": \"lock\", \"lockid\": %d}", i);
            return 1;
        case ANANKE_UNLOCK_RESOURCE:
            if (!pair_get_value_at(root, "lockid", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "lockid");
                return 1;
            }
            if (vtype != AK_ENC_INTEGER) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "lockid(integer)");
                return 1;
            }
            if (!unlock(session->lockCtx, *((int *)value))) {
                lwsl_err("Cannot unlock resource\n");
            }
            ananke_message(session, "{\"operation\": \"unlock\", \"done\": true}");
            return 1;
        case ANANKE_SET_LOCALE:
            if (!pair_get_value_at(root, "locale", &vtype, &value, &vlen)) {
                ananke_error(session ,AK_ERR_MISSING_ARGUMENT, "locale");
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "locale(string)");
                return 1;
            }
            i = 0;
            while (ErrorMapLocale[i].lang != NULL) {
                if (strncmp(ErrorMapLocale[i].lang, (char *)value, vlen) == 0) {
                    if (mlock(&(session->mutex))) {
                        session->errmap = ErrorMapLocale[i].errmap;
                        munlock(&(session->mutex));
                        success = 1;
                        break;
                    }
                }
                i++;
            }
            if (!success) {
                ananke_error(session, AK_ERR_OPERATION_FAILED, OperationMap[opid].opstr);
            } else {
                ananke_error(session, AK_ERR_SUCCESS, OperationMap[opid].opstr);
            }
            return 1;
    }
}