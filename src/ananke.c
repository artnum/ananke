#include "include/ananke.h"
#include <string.h>

/* must be locked before */
void ananke_error (Session * session, AnankeErrorCode code, const char * details, char * clientOpId) {
    Message * msg = NULL;
    int i = 0;
    struct _s_anankeErrorMap *errmap = (struct _s_anankeErrorMap *)&EnErrorMap;

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
        "{\"operation\": \"error\", \"message\": \"%s\", \"code\": %d, \"details\": \"%s\", \"request-id\": \"%s\"}",
        errmap[i].msg,
        errmap[i].code, details != NULL ? details : "",
        clientOpId == NULL ? "" : clientOpId)) { 
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
    AKType vtype;
    Message * msg = NULL;
    size_t vlen;
    char * clientOpId = NULL;
    int i = 0;
    int success = 0;
    void * value;
    int opid = -1;
    char * str = NULL;

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
        ananke_error(session, AK_ERR_UNKNOWN_OP, NULL, NULL);
        return 1;
    }

    if (!pair_get_value_at(root, "opid", &vtype, &value, &vlen)) {
        ananke_error(session, AK_ERR_MISSING_OPID, "opid(string)", NULL);
        return 1;
    }
    if (vtype != AK_ENC_STRING) {
        ananke_error(session, AK_ERR_WRONG_OPID_TYPE, "opid(string)", NULL);
        return 1;
    }

    clientOpId = strndup((const char *)value, vlen);
    if (!clientOpId) {
        ananke_error(session, AK_ERR_SERVER_ERROR, "censored by twitter", NULL);
        return 1;
    }
    msg = msg_new_pointer(clientOpId, NULL);
    if (!msg) {
        ananke_error(session, AK_ERR_SERVER_ERROR, "censored by twitter", clientOpId);
        free(clientOpId);
        return 1;
    }
    if (!msgstack_push(&(session->requestId), msg, NULL)) {
        ananke_error(session, AK_ERR_SERVER_ERROR, "censored by twitter", clientOpId);
        msg_free(msg);
        return 1;
    }

    switch (OperationMap[opid].operation) {
        default:
        case ANANKE_NOP: break;
        case ANANKE_CLOSE:
            if (!mlock(&(session->mutex))) { return -1; }
            session->end = 1;
            munlock(&(session->mutex));
            mcondsignal(&(session->condition));
            return 0;
        case ANANKE_PING:
            ananke_error(session, AK_ERR_NOT_ALLOWED, NULL, clientOpId);
            return 1;
        case ANANKE_PONG:
            if (!pair_get_value_at(root, "count", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "count", clientOpId);
                return 1;
            }
            if (vtype != AK_ENC_INTEGER) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "count(integer)", clientOpId);
                return 1;
            }
            if (!mlock(&(session->mutex))) { return -1; }
            session->pongReceived = *((int *)value);
            munlock(&(session->mutex));
            return 0;
        case ANANKE_LOCK_RESOURCE:
            if (!pair_get_value_at(root, "resource", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "resource", clientOpId);
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "resource(string)", clientOpId);
                return 1;
            }
            str = lock(session->lockCtx, (char *)value, vlen, NULL);
            if (str) {
                ananke_message(session, "{\"operation\": \"lock\", \"lockid\": \"%s\", \"request-id\": \"%s\"}", str, clientOpId);
            } else {
                ananke_message(session, "{\"operation\": \"lock\", \"busy\": \"%s\", \"request-id\": \"%s\"}", (char *)value, clientOpId);
            }
            free(str);
            str = NULL;
            return 1;
        case ANANKE_UNLOCK_RESOURCE:
            if (!pair_get_value_at(root, "lockid", &vtype, &value, &vlen)) {
                ananke_error(session, AK_ERR_MISSING_ARGUMENT, "lockid", clientOpId);
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "lockid(string)", clientOpId);
                return 1;
            }

            unlock(session->lockCtx, (char *)value);
            ananke_message(session, "{\"operation\": \"unlock\", \"done\": true, \"request-id\": \"%s\"}", clientOpId);
            return 1;
        case ANANKE_SET_LOCALE:
            if (!pair_get_value_at(root, "locale", &vtype, &value, &vlen)) {
                ananke_error(session ,AK_ERR_MISSING_ARGUMENT, "locale", clientOpId);
                return 1;
            }
            if (vtype != AK_ENC_STRING) {
                ananke_error(session, AK_ERR_WRONG_TYPE, "locale(string)", clientOpId);
                return 1;
            }
            i = 0;
            while (ErrorMapLocale[i].lang != NULL) {
                if (strncmp(ErrorMapLocale[i].lang, (char *)value, vlen) == 0) {
                    if (mlock(&(session->mutex))) {
                        session->errmap = (struct _s_anankeErrorMap *)ErrorMapLocale[i].errmap;
                        munlock(&(session->mutex));
                        success = 1;
                        break;
                    }
                }
                i++;
            }
            if (!success) {
                ananke_error(session, AK_ERR_OPERATION_FAILED, OperationMap[opid].opstr, clientOpId);
            } else {
                ananke_error(session, AK_ERR_SUCCESS, OperationMap[opid].opstr, clientOpId);
            }
            return 1;
    }
    return 0;
}