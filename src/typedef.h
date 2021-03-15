#ifndef TYPEDEF_H__
#define TYPEDEF_H__

typedef enum {
    AK_ENC_NONE = -1,
    AK_ENC_BOOL = 1,
    AK_ENC_NULL,
    AK_ENC_STRING,
    AK_ENC_INTEGER,
    AK_ENC_FLOAT,
    AK_ENC_BYTESTRING,
    AK_ENC_OBJECT
} AKType;

typedef enum {
    AK_MSG_STRING = 0,
    AK_MSG_POINTER
} AKMsgType;

#endif /* TYPEDEF_H__ */