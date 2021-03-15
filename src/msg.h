#ifndef MSG_H__
#define MSG_H__

#include <libwebsockets.h>
#include <stddef.h>
#include <pthread.h>
#include "typedef.h"

typedef struct _s_message Message;
struct _s_message {
    char * body;
    AKMsgType type;
    size_t len;
    size_t cursor;
    int nulled;
    Message * next;
    Message * previous;
};

typedef struct _s_message_stack {
    Message * first;
    Message * last;
    size_t count;
    pthread_mutex_t mutex;
} MessageStack;

typedef enum {
    AK_STACK_SUCCESS = 0,
    AK_STACK_LOCK = 1,

    AK_WHY_DO_I_PASS_NULL_STACK_AS_ARGUMENT,
    AK_WHY_DO_I_PASS_NULL_MESSAGE_ON_STACK_AS_ARGUMENT
} AKStackError;

#define msg_length(msg) ((msg)->nulled ? (msg)->len - LWS_SEND_BUFFER_POST_PADDING - 1 : (msg)->len - LWS_SEND_BUFFER_POST_PADDING)

Message * msg_new (AKMsgType type);
int msg_append (Message * msg, char * content, size_t len);
void msg_free (Message * msg);
size_t msg_printf(Message * msg, const char * format, ...);
size_t msg_vprintf(Message * msg, const char * format, va_list ap);
void msgstack_init (MessageStack * stack);
int msgstack_push (MessageStack * stack, Message * msg, AKStackError * err);
Message * msgstack_pop (MessageStack * stack, AKStackError * err);
size_t msgstack_count(MessageStack * stack);

#endif /* MSG_H__ */