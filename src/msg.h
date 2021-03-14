#ifndef MSG_H__
#define MSG_H__

#include <libwebsockets.h>
#include <stddef.h>
#include <pthread.h>

typedef struct _s_message Message;
struct _s_message {
    char * body;
    size_t len;
    size_t cursor;
    int nulled;
    Message * next;
    pthread_mutex_t mutex;
};

#define msg_length(msg) ((msg)->nulled ? (msg)->len - LWS_SEND_BUFFER_POST_PADDING - 1 : (msg)->len - LWS_SEND_BUFFER_POST_PADDING)

Message * msg_new ();
int msg_append (Message * msg, char * content, size_t len);
void msg_free (Message * msg);
int msg_stack_push (Message ** stack, Message * msg);
int msg_printf(Message * msg, const char * format, ...);
Message * msg_stack_pop (Message ** stack);
int msg_stack_size(Message * stack);

#endif /* MSG_H__ */