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
    minit(&(new->mutex));
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

int msg_printf(Message * msg, const char * format, ...) {
    int len = 0;
    va_list args;

    if (msg == NULL) { return 0; }
    /* pre-flight */
    if(!mlock(&(msg->mutex))) { return 0; }
    va_start(args, format);
    len = vsnprintf(NULL, 0, format, args) + 1;
    if (len == 0) { return 0; }
    if (!_msg_realloc(msg, len)) { munlock(&(msg->mutex)); return 0; }
    va_start(args, format);
    if (len != vsnprintf(msg->body + msg->cursor, len, format, args) + 1) { munlock(&(msg->mutex)); return 0; }
    msg->cursor += len;
    msg->nulled = 1;
    munlock(&(msg->mutex));

    return len;
}

/* keep space for lws padding, msg->body is set at then end of pre-padding */
int msg_append (Message * msg, char * content, size_t len) {
    int i = 0;

    if (len == 0) { return 0; }
    if (!mlock(&(msg->mutex))) { return 0; }
    if (!_msg_realloc(msg, len)) { munlock(&(msg->mutex)); return 0; }
    memcpy((msg->body + msg->cursor), content, len);
    msg->cursor += len;
    munlock(&(msg->mutex));
    return 1;
}

void msg_free (Message * msg) {
    if (msg) {
        if (!mlock(&(msg->mutex))) { return; }
        mdestroy(&(msg->mutex));
        if (msg->body) { free(msg->body - LWS_PRE); }
        free(msg);
    }
}

/* just lock the stack during manipulation as it is the entry point */
int msg_stack_push (Message ** stack, Message * msg) {
    if (stack == NULL) { return 0; }
    if (msg == NULL) { return 0; }
    if (*stack != NULL) { if (!mlock(&((*stack)->mutex))) { return 0; } }
    msg->next = *stack;
    *stack = msg;
    if (msg->next != NULL) { munlock(&(msg->next->mutex)); }
    return 1;
}

Message * msg_stack_pop (Message ** stack) {
    Message * current = NULL;
    Message * previous = NULL;
    if (stack == NULL) { return NULL; }
    if (*stack == NULL) { return NULL; }
    if (!mlock(&((*stack)->mutex))) { return NULL; }
    current = *stack;
    while (current->next != NULL) {
        previous = current;
        current = current->next;
    }

    if (previous) { previous->next = NULL; }
    if (current == *stack) { *stack = NULL; }
    /* if current is stack, we have current locked */
    if (*stack != NULL) { munlock(&((*stack)->mutex)); } else { munlock(&(current->mutex)); }
    return current;
}

int msg_stack_size (Message * stack) {
    Message * current = stack;
    if (stack == NULL) { return 0; }
    int count = 0;
    if (!mlock(&(stack->mutex))) { return 0;}
    while (current != NULL) {
        count++;
        current = current->next;
    }
    munlock(&(stack->mutex));
    return count;
}