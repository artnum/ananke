#include "ananke.h"
#include "lock.h"
#include "msg.h"
#include "prot.h"
#include "mutex.h"
#include <signal.h>
#include <stdio.h>
#include <libwebsockets.h>
#include <stdio.h>
#include <unistd.h>

LockContext * lockCtx = NULL;

void * process_protocol (void * d) {
    Session * session = (Session *)d;
    Pair * root = NULL;
    Pair * current = NULL;
    Message * msg;
    void * value = NULL;
    Type vtype = NONE;
    size_t vlen = 0;
    int end = 0;
    int opret = 0;
    do {
        if (!mlock(&(session->mutex))) { continue; }
        lwsl_notice("THREAD WAIT\n");
        mcondwait(&(session->condition), &(session->mutex));
        end = session->end;
        munlock(&(session->mutex));
        if (!mlock(&(session->min))) { lwsl_err("Instack locked\n"); continue; }
        msg = msg_stack_pop(&(session->instack));
        munlock(&(session->min));
        if (msg != NULL) {
            root = proto_parse(msg);
            if (root) {
                opret = ananke_operation(root, session);
                if (opret > 0) {
                    lws_callback_on_writable(session->wsi);
                } else if (opret < 0) {
                    lwsl_err("Unrecoverable error\n");
                }
            }
            msg_free(msg);
            pair_free(root);
        }
    } while(end == 0);

    pthread_exit(NULL);
    return NULL;
}

int ananke_protocol (struct lws * wsi, enum lws_callback_reasons reason, void * user, void *in, size_t len) {
    Session *session = (Session *)user;
    Message * msg = NULL;
    char * body = NULL;
    Pair * message = NULL;
    switch (reason) {
        default: break;
        case LWS_CALLBACK_PROTOCOL_INIT:
            lwsl_notice("Setup\n");
            break;
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_notice("New client\n");
            minit(&(session->mutex));
            minit(&(session->mout));
            minit(&(session->min));
            mcondinit(&(session->condition));
            pthread_create(&(session->userthread), NULL, process_protocol, session);
            session->pingCount = 0;
            session->pongReceived = 0;
            session->end = 0;
            session->wsi = wsi;
            session->lockCtx = lockCtx;
            msg = msg_new();
            if (!msg) {
                lwsl_err("Cannot allocate message\n");
                return 1;
            }
            if (!msg_printf(msg, "{\"operation\": \"connected\", \"state\": \"OK\"}")) {
                lwsl_err("Cannot fill message\n");
                return 1;
            }
            if (mlock(&(session->mout))) {
                msg_stack_push(&(session->outstack), msg);
                munlock(&(session->mout));
            }
            lws_callback_on_writable(wsi);
            lws_set_timer_usecs(wsi, 5000000);
            break;
        /* ping */
        case LWS_CALLBACK_TIMER:
            lwsl_notice("Timer\n");
            session->pingCount++;
            if (session->end) {
                lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
                return 1;
            }
            if (session->pingCount - session->pongReceived > 10) {
                session->end = 1;
                lwsl_notice("Client disconnected\n");
                lws_close_reason(wsi, LWS_CLOSE_STATUS_PROTOCOL_ERR, NULL, 0);
                return 1;
            }
            msg = msg_new();
            if (!msg) { lwsl_err("Cannot allocate message\n"); return 1; }
            if (!msg_printf(msg, "{\"operation\": \"ping\", \"count\": %d}", session->pingCount)) { return 1; }
            if(mlock(&(session->mout))) {
                msg_stack_push(&(session->outstack), msg);
                munlock(&(session->mout));
            } else {
                msg_free(msg);
            }
            lws_callback_on_writable(wsi);
            lws_set_timer_usecs(wsi, 5000000);
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
            lwsl_notice("Close\n");
            session->end = 1;
            mcondsignal(&(session->condition));
            pthread_join(session->userthread, NULL);
            lwsl_notice("Thread joined. End is near\n");
            if(mlock(&(session->min))) {
                while((msg = msg_stack_pop(&(session->instack))) != NULL) {
                    msg_free(msg);
                }
                munlock(&(session->min));
            }
            if(mlock(&(session->mout))) {
                while((msg = msg_stack_pop(&(session->outstack))) != NULL) {
                    msg_free(msg);
                }
                munlock(&(session->mout));
            }
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (mlock(&(session->mout))) {
                lwsl_notice("Write\n");
                msg = msg_stack_pop(&(session->outstack));
                if (msg) {
                    lwsl_notice("Message to output\n");
                    lws_write(wsi, (unsigned char *)msg->body, msg_length(msg) , LWS_WRITE_TEXT);
                    msg_free(msg);
                    msg = NULL;
                    if (msg_stack_size(&(session->outstack)) > 0) {
                        lwsl_notice("More message pending\n");
                        lws_callback_on_writable(wsi);
                    }
                }
                munlock(&(session->mout));
            } else {
                lws_callback_on_writable(wsi);
            }
            break;
        case LWS_CALLBACK_RECEIVE:
            lwsl_notice("Read\n");
            if (lws_is_first_fragment(wsi)) {
                msg = msg_new();
                if (msg == NULL) {
                    lwsl_err("Cannot allocate new message\n");
                    return 1; 
                }
                session->current = msg;
            } else {                
                msg = session->current;
                if (msg == NULL) { 
                    lwsl_err("Current message is null\n");
                    return 1; 
                }
            }
            if (len <= 0) { 
                return 1; 
            }
            if (!msg_append(msg, (char *)in, len)) {
                lwsl_err("Message append failed: DROP\n");
                msg_free(msg);
                session->current = NULL;
                return 1;
            }
            if (lws_is_final_fragment(wsi)) {
                session->current = NULL;
                if (mlock(&(session->min))) {
                    if (!msg_stack_push(&(session->instack), msg)) {
                        lwsl_err("Cannot add to stack: DROP\n");
                        msg_free(msg);
                        munlock(&(session->min));
                        return 1;
                    }
                    munlock(&(session->min));
                } else {
                    msg_free(msg);
                }
                mcondsignal(&(session->condition));
            }
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {"ananke", ananke_protocol, sizeof(Session), 128, 0, NULL, 0},
    {NULL, NULL, 0, 0}
};

static const struct lws_protocol_vhost_options pvo = {
    NULL, NULL, "ananke", ""
};

static int interrupted = 0;

void sigint_handler (int sig) {
    interrupted = 1;
}

int main (int argc, char ** argv) {
    int n = 1;
    struct lws_context_creation_info info;
    struct lws_context * context;

    signal(SIGINT, sigint_handler);

    memset(&info, 0, sizeof(info));
    info.port = 9991;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;
    info.pvo = &pvo;
    info.vhost_name = "localhost";

    context = lws_create_context(&info);
    if (!context) { lwsl_err("ananke init failed"); return 1; }
    
    lockCtx = lock_ctx_init();
    while(n >= 0 && interrupted == 0) {
        n = lws_service(context, 0);
    }

    lwsl_info("Ending\n");
    lws_context_destroy(context);

    
    lock_ctx_destroy(lockCtx);
    return 0;
}