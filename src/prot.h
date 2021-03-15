#ifndef PROT_H__
#define PROT_H__

#include "msg.h"
#include <stddef.h>
#include <stdint.h>

/* BER inspired protocol :
 * +---+--------+--------+---//---+---//---+
 * | T | LN     | LV     | NAME   | VALUE  |
 * +---+--------+--------+---//---+---//---+
 * 
 * T     : Type, 3 char
 * LN    : Length of name, base36 encoded, 8 digit 0 padded
 * LV    : Length of value, base36 encoded, 8 digit 0 padded
 * NAME  : The name of the entry (utf-8)
 * VALUE : The value of the entry
 * 
 * Types
 * -----
 * STR -> utf-8 string
 * INT -> integer
 * FLO -> float value
 * BOO -> bool value (value is 1 byte '1' or '0')
 * NUL -> null value (value is 0 byte)
 * HEX -> byte string in hex encoding
 * BST -> byte string
 * OBJ -> a set of value of any type. LV is the complete length of the encoded
 *        set. Array are set of value too, names are just 0, 1, 2, 3, ...
 *
 * Limits
 * ------
 * Name and value can be above 2.5Tib.
 * 
 */

typedef enum {
    AK_ENC_NONE = -1,
    AK_ENC_BOOL = 1,
    AK_ENC_NULL,
    AK_ENC_STRING,
    AK_ENC_INTEGER,
    AK_ENC_FLOAT,
    AK_ENC_BYTESTRING,
    AK_ENC_OBJECT
} Type;

typedef struct _s_pair Pair;
typedef struct _s_object Object;

struct _s_pair {
    char * name;
    Type type;
    size_t length;
    int64_t iv;
    double fv;
    char * sv;
    Object * ov;
};
struct _s_object {
    Pair ** list;
    int count;
    Pair * parent;
    size_t end;
};

Object * object_new (void);
int object_add_pair (Object * object, Pair * pair);
void object_print (Object * object, int level);
void object_free (Object * object);
Pair * pair_new (char * name, size_t nlen, char * type, char * value, size_t vlen);
void pair_print (Pair * pair, int level);
Pair * pair_at(Pair * pair, const char * path);
int pair_get_value (Pair * pair, Type * vtype, void ** value, size_t *vlen);
int pair_get_value_at (Pair * pair, const char * path, Type * vtype, void ** value, size_t * vlen);
void pair_free (Pair * pair);
Pair * proto_parse (Message * msg);

#endif /* PROT_H__ */