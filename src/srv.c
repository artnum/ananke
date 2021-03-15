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
    int end = 0;
    int opret = 0;
    int i = 0;
    size_t vlen = 0;
    AKStackError sterr = AK_STACK_SUCCESS;
    AKType vtype = AK_ENC_NONE;

    do {
        if (!mlock(&(session->mutex))) {
            i++;
            if (i > 50) { goto failed_as_hell; }
            continue; 
        }
        i = 0;

        mcondwait(&(session->condition), &(session->mutex));
        end = session->end;
        munlock(&(session->mutex));

        msg = msgstack_pop(&(session->in), &sterr);
        if (sterr == AK_STACK_LOCK) { lwsl_warn("Stack lock stalled(%s:%d)\n", __FILE__, __LINE__); }

        if (msg == NULL) { continue; }
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
    } while(end == 0);

    pthread_exit(NULL);
    return NULL;

failed_as_hell:
    lwsl_err("Thread failed to get the session lock for too long\n");
}

int ananke_protocol (struct lws * wsi, enum lws_callback_reasons reason, void * user, void *in, size_t len) {
    Session *session = (Session *)user;
    Message * msg = NULL;
    char * body = NULL;
    Pair * message = NULL;
    int pingDiff = 0;
    AKStackError sterr = AK_STACK_SUCCESS;
    int i = 0;
    switch (reason) {
        default: break;
        case LWS_CALLBACK_PROTOCOL_INIT:
            lwsl_notice("Setup\n");
            break;
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_notice("New client\n");
            minit(&(session->mutex));
            msgstack_init(&(session->in));
            msgstack_init(&(session->out));
            mcondinit(&(session->condition));
            pthread_create(&(session->userthread), NULL, process_protocol, session);
            session->pingCount = 0;
            session->pongReceived = 0;
            session->end = 0;
            session->wsi = wsi;
            session->lockCtx = lockCtx;
            session->errmap = &EnErrorMap;

            ananke_message(
                session, 
                "{\"operation\": \"connection\", \"state\": true, \"ping-inverval\": %d}", 
                AK_PING_INTERVAL
                );
            lws_set_timer_usecs(wsi, AK_PING_INTERVAL);

            break;
        /* ping */
        case LWS_CALLBACK_TIMER:
            lwsl_notice("Timer\n");
            if (mlock(&(session->mutex))) {
                session->pingCount++;
                pingDiff = session->pingCount - session->pongReceived;
                munlock(&(session->mutex));
            } else {
                lwsl_err("session mutex failed\n");
            }
            if (session->end) {
                lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
                return 1;
            }
            
            if (pingDiff > 10) {
                session->end = 1;
                lwsl_notice("Client disconnected\n");
                lws_close_reason(wsi, LWS_CLOSE_STATUS_PROTOCOL_ERR, NULL, 0);
                return 1;
            }

            ananke_message(session, "{\"operation\": \"ping\", \"count\": %d}", session->pingCount);

            lws_set_timer_usecs(wsi, AK_PING_INTERVAL);
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
            lwsl_notice("Close\n");
            session->end = 1;
            mcondsignal(&(session->condition));
            pthread_join(session->userthread, NULL);
            lwsl_notice("Thread joined. Doing great effort to clean up before death !\n");

            i = 0;
            do {
                while((msg = msgstack_pop(&(session->in), &sterr)) != NULL) {
                    msg_free(msg);
                }
                i++;
            } while (sterr != AK_STACK_SUCCESS && i < 50);
            i = 0;
            do {
                while((msg = msgstack_pop(&(session->out), &sterr)) != NULL) {
                    msg_free(msg);
                }
                i++;
            } while (sterr != AK_STACK_SUCCESS && i < 50);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
                msg = msgstack_pop(&(session->out), &sterr);
                if (!msg) { if (sterr == AK_STACK_LOCK) { lws_callback_on_writable(wsi); } break; }
                lws_write(wsi, (unsigned char *)msg->body, msg_length(msg) , LWS_WRITE_TEXT);
                msg_free(msg);
                msg = NULL;
                if (msgstack_count(&(session->out)) > 0) {
                    lws_callback_on_writable(wsi);
                }
            break;
        case LWS_CALLBACK_RECEIVE:
            lwsl_notice("Read\n");
            if (lws_is_first_fragment(wsi)) {
                msg = msg_new(AK_MSG_STRING);
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
                msgstack_push(&(session->in), msg, &sterr);
                if (sterr != AK_STACK_SUCCESS) {
                    lwsl_err("Cannot add to stack: DROP\n");
                    msg_free(msg);
                    return 1;
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