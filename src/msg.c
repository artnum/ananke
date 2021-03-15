#include "msg.h"
#include "mutex.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

Message * msg_new () {
    Message * new = NULL;

    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }
    new->body = NULL;
    new->len = 0;
    new->cursor = 0;
    new->next = NULL;
    new->nulled = 0;
    return new;
}

int _msg_realloc(Message * msg, size_t addLen) {
    int newLength = 0;
    char * newSpace = NULL;
    int i = 0;
    newLength = msg->len + addLen;
    if (newLength <= 0) { return 1; }
    newSpace = realloc(msg->body == NULL ? NULL : msg->body - LWS_PRE, (sizeof(*newSpace) * newLength) + LWS_PRE + LWS_SEND_BUFFER_POST_PADDING);
    if (newSpace == NULL) { return 0; }
    msg->body = newSpace + LWS_PRE;
    msg->len = newLength;
    /* set to 0 new memory space */
    for (i = msg->len; i < newLength; i++) {
        *(msg->body + i) = 0;
    }
    return 1;
}

size_t msg_vprintf(Message * msg, const char * format, va_list ap) {
    size_t len = 0;
    va_list ap2;

    va_copy(ap2, ap);
    if (msg == NULL) { return 0; }
    /* pre-flight */
    len = vsnprintf(NULL, 0, format, ap) + 1;
    if (len == 0) { return 0; }
    if (!_msg_realloc(msg, len)) { return 0; }
    if (len != vsnprintf(msg->body + msg->cursor, len, format, ap2) + 1) { return 0; }
    msg->cursor += len;
    msg->nulled = 1;

    return len;    
}

size_t msg_printf(Message * msg, const char * format, ...) {
    size_t len = 0;
    va_list args;

    if (msg == NULL) { return 0; }
    if (format == NULL) { return 0; }

    va_start(args, format);
    len = msg_vprintf(msg, format, args);
    va_end(args);
    return len;
}

/* keep space for lws padding, msg->body is set at then end of pre-padding */
int msg_append (Message * msg, char * content, size_t len) {
    int i = 0;

    if (len == 0) { return 0; }
    if (!_msg_realloc(msg, len)) { munlock(&(msg->mutex)); return 0; }
    memcpy((msg->body + msg->cursor), content, len);
    msg->cursor += len;
    return 1;
}

void msg_free (Message * msg) {
    if (msg) {
        if (msg->body) { free(msg->body - LWS_PRE); }
        free(msg);
    }
}

/* just lock the stack during manipulation as it is the entry point */
int msg_stack_push (Message ** stack, Message * msg) {
    if (stack == NULL) { return 0; }
    if (msg == NULL) { return 0; }
    msg->next = *stack;
    *stack = msg;
    return 1;
}

Message * msg_stack_pop (Message ** stack) {
    Message * current = NULL;
    Message * previous = NULL;
    if (stack == NULL) { return NULL; }
    if (*stack == NULL) { return NULL; }
    current = *stack;
    while (current->next != NULL) {
        previous = current;
        current = current->next;
    }

    if (previous) { previous->next = NULL; }
    if (current == *stack) { *stack = NULL; }
    /* if current is stack, we have current locked */
    return current;
}

int msg_stack_size (Message * stack) {
    Message * current = stack;
    if (stack == NULL) { return 0; }
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}