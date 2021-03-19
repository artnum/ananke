#include "include/msg.h"
#include "include/mutex.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

Message * msg_new (AKMsgType type) {
    Message * new = NULL;

    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }
    new->body = NULL;
    new->len = 0;
    new->cursor = 0;
    new->next = NULL;
    new->previous = NULL;
    new->free = NULL;
    new->nulled = 0;
    new->type = type;
    return new;
}

Message * msg_new_pointer (void * ptr, void (*cbfree)(void *)) {
    Message * new = NULL;
    new = calloc(1, sizeof(*new));
    new->body = ptr;
    new->type = AK_MSG_POINTER;
    new->len = sizeof(ptr);
    new->cursor = new->len;
    new->next = NULL;
    new->previous = NULL;
    new->nulled = 0;
    if (cbfree == NULL) {
        new->free = free;
    } else {
        new->free = cbfree;
    }
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

    if (msg == NULL) { return 0; }
    if (msg->type != AK_MSG_STRING) { return 0; }
    if (format == NULL) { return 0; }

    va_copy(ap2, ap);
    if (msg == NULL) { return 0; }
    /* pre-flight */
    len = (size_t)vsnprintf(NULL, 0, format, ap) + 1;
    if (len == 0) { return 0; }
    if (!_msg_realloc(msg, len)) { return 0; }
    if (len != (size_t)vsnprintf(msg->body + msg->cursor, len, format, ap2) + 1) { return 0; }
    msg->cursor += len;
    msg->nulled = 1;

    return len;    
}

size_t msg_printf(Message * msg, const char * format, ...) {
    size_t len = 0;
    va_list args;

    if (msg == NULL) { return 0; }
    if (msg->type != AK_MSG_STRING) { return 0; }
    if (format == NULL) { return 0; }

    va_start(args, format);
    len = msg_vprintf(msg, format, args);
    va_end(args);
    return len;
}

/* keep space for lws padding, msg->body is set at then end of pre-padding */
int msg_append (Message * msg, char * content, size_t len) {
    if (msg == NULL) { return 0; }
    if (msg->type != AK_MSG_STRING) { return 0; }
    if (content == NULL) { return 0; }

    if (len == 0) { return 0; }
    if (!_msg_realloc(msg, len)) { return 0; }
    memcpy((msg->body + msg->cursor), content, len);
    msg->cursor += len;
    return 1;
}

void msg_free (Message * msg) {
    if (msg) {
        if (msg->body) { 
            if (msg->free) {
                msg->free(msg->body);
            } else {
                free(msg->body - LWS_PRE);
            }
        }
        free(msg);
    }
}

void msgstack_init (MessageStack * stack) {
    if (stack == NULL) { return; }
    stack->first = NULL;
    stack->last = NULL;
    stack->count = 0;
    minit(&(stack->mutex));
}

int msgstack_push (MessageStack * stack, Message * msg, AKStackError * err) {
    if (stack == NULL) { if (err) { *err = AK_WHY_DO_I_PASS_NULL_STACK_AS_ARGUMENT; } return 0; }
    if (msg == NULL) { if (err) { *err = AK_WHY_DO_I_PASS_NULL_MESSAGE_ON_STACK_AS_ARGUMENT; }return 0; }

    if (!mlock(&(stack->mutex))) { if (err) { *err = AK_STACK_LOCK; } return 0; }
    if (stack->first == stack->last) {
        msg->next = stack->first;
        msg->previous = NULL;
        stack->first = msg;
        if (stack->last == NULL) { stack->last = msg; }
        else { stack->last->previous = msg; }
        stack->count++;
        munlock(&(stack->mutex));
        if (err) { *err = AK_STACK_SUCCESS; }
        return 1;
    }

    msg->next = stack->first;
    stack->first = msg;
    stack->count++;
    munlock(&(stack->mutex));
    if (err) { *err = AK_STACK_SUCCESS; }
    return 1;
}

Message * msgstack_pop (MessageStack * stack, AKStackError * err ) {
    Message * msg = NULL;
    if (stack == NULL) { if (err) { *err = AK_WHY_DO_I_PASS_NULL_STACK_AS_ARGUMENT; } return NULL; }

    if (!mlock(&(stack->mutex))) { if(err) { *err = AK_STACK_LOCK; } return NULL; }
    if (stack->first == stack->last) {
        msg = stack->first;
        if (msg == NULL) {
            /* empty stack is not a failure, it's an empty stack */
            munlock(&(stack->mutex));
            if (err) { *err = AK_STACK_SUCCESS; }
            return msg;
        }
        stack->first = NULL;
        stack->last = NULL;
        msg->next = NULL;
        msg->previous = NULL;
        stack->count--;
        munlock(&(stack->mutex));
        if (err) { *err = AK_STACK_SUCCESS; }
        return msg;
    }
    msg = stack->last;
    msg->previous->next = NULL;
    stack->last = msg->previous;
    msg->next = NULL;
    msg->previous = NULL;
    munlock(&(stack->mutex));
    if (err) { *err = AK_STACK_SUCCESS; }
    return msg;
}

size_t msgstack_count(MessageStack * stack) {
    size_t size = 0;
    if (stack == NULL) { return 0; }
    if (!mlock(&(stack->mutex))) { return 0; }
    size = stack->count;
    munlock(&(stack->mutex));
    return size;
}