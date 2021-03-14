#include "prot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* The protocol don't need to be decoded first. The way it is layed out
 * it is possible to operate on the message directly.
 */

Object * object_new (void) {
    Object * new = NULL;
    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }
    new->count = 0;
    new->list = NULL;
    new->parent = NULL;
    new->end = 0;
    return new;
}

int object_add_pair (Object * object, Pair * pair) {
    int newCount = 0;
    void * newList = NULL;
    if (object == NULL) { return 0; }
    newCount = object->count + 1;
    newList = realloc(object->list, newCount * sizeof(*(object->list)));
    if (newList == NULL) { return 0; }
    object->list = newList;
    object->count = newCount;
    *(object->list + object->count - 1) = pair;
    return 1;
}

void object_print (Object * object, int level) {
    int i = 0;
    int j = 0;
    printf("(object){\n");
    for (i = 0; i < object->count; i++) {
        if (*(object->list + i) == NULL) {
            for (j = 0; j < level * 3; j++) { printf(" "); }
            printf("(null)\n");
        } else {
            pair_print(*(object->list + i), level);
        }
    }
    for (j = 0; j < (level - 1) * 3; j++) { printf(" "); }
    printf("}\n");
}

void object_free (Object * object) {
    int i = 0;
    if (object == NULL) { return; }

    for (i = 0; i < object->count; i++) {
        pair_free(*(object->list + i));
    }
    free(object->list);
    free(object);
}

size_t toSize (char * value, size_t vlen, char ** error) {
    size_t out = 0;
    size_t i = 0;
    char * err = NULL;

    for (i = 0; i < vlen; i++) {
        out *= 36;
        switch (*(value + i)) {
            case '0': break;
            case '1': out += 1; break;
            case '2': out += 2; break;
            case '3': out += 3; break;
            case '4': out += 4; break;
            case '5': out += 5; break;
            case '6': out += 6; break;
            case '7': out += 7; break;
            case '8': out += 8; break;
            case '9': out += 9; break;
            case 'a': case 'A': out += 10; break;
            case 'b': case 'B': out += 11; break;
            case 'c': case 'C': out += 12; break;
            case 'd': case 'D': out += 13; break;
            case 'e': case 'E': out += 14; break;
            case 'f': case 'F': out += 15; break;
            case 'g': case 'G': out += 16; break;
            case 'h': case 'H': out += 17; break;
            case 'i': case 'I': out += 18; break;
            case 'j': case 'J': out += 19; break;
            case 'k': case 'K': out += 20; break;
            case 'l': case 'L': out += 21; break;
            case 'm': case 'M': out += 22; break;
            case 'n': case 'N': out += 23; break;
            case 'o': case 'O': out += 24; break;
            case 'p': case 'P': out += 25; break;
            case 'q': case 'Q': out += 26; break;
            case 'r': case 'R': out += 27; break;
            case 's': case 'S': out += 28; break;
            case 't': case 'T': out += 29; break;
            case 'u': case 'U': out += 30; break;
            case 'v': case 'V': out += 31; break;
            case 'w': case 'W': out += 32; break;
            case 'x': case 'X': out += 33; break;
            case 'y': case 'Y': out += 34; break;
            case 'z': case 'Z': out += 35; break;
            default: err = value + i; break;
        }
        if (err) {
            break;
        }
    }

    if (err && error) {
        *error = err;
    }
    return out;
}

double toFloat (char * value, size_t vlen, char ** error) {
    double out = 0;
    size_t i = 0;
    double fcnt = 0;
    int neg = 0;
    int floating = 0;
    char * err = NULL;

    for (i = 0; i < vlen; i++) {
        if (floating == 0) { out *= 10; } else { fcnt++; }
        switch(*(value + i)) {
            case '-':
                if (i == 0) { neg = 1; } else { err = value + i; }
                break;
            case '+':
                if (i == 0) { neg = 0; } else { err = value + i; }
                break;
            case '0': break;
            case '1': 
                if (floating) {
                    out += (1.0 / pow(10, fcnt));
                } else {
                    out += 1;
                }
                break;
            case '2': 
                if (floating) {
                    out += (2.0 / pow(10, fcnt));
                } else {
                    out += 2;
                }
                break;
            case '3': 
                if (floating) {
                    out += (3.0 / pow(10, fcnt));
                } else {
                    out += 3;
                }
                break;
            case '4':
                if (floating) {
                    out += (4.0 / pow(10, fcnt));
                } else {
                    out += 4; 
                }
                break;
            case '5': 
                if (floating) {
                    out += (5.0 / pow(10, fcnt));
                } else {
                    out += 5;
                }
                break;
            case '6': 
                if (floating) {
                    out += (6.0 / pow(10, fcnt));
                } else {
                    out += 6; 
                }
                break;
            case '7': 
                if (floating) {
                    out += (7.0 / pow(10, fcnt));
                } else {
                    out += 7;
                }
                break;
            case '8': 
                if (floating) {
                    out += (8.0 / pow(10, fcnt));
                } else {
                    out += 8;
                }
                break;
            case '9': 
                if (floating) {
                    out += (9.0 / pow(10, fcnt));
                } else {
                    out += 9; 
                }
                break;
            case '.': case ',': 
                if (floating) { err = value + i; }
                out /= 10;
                floating = 1; fcnt = 0; 
                break;
            default: err = value + i; break;
        }
        if (err) { break; }
    }
    if (err && error != NULL) {
        *error = err;
    }
    return neg == 1 ? -out : out;    
}

int64_t toInt (char * value, size_t vlen, char ** error) {
    int64_t out = 0;
    size_t i = 0;
    int neg = 0;
    char * err = NULL;

    for (i = 0; i < vlen; i++) {
        out *= 10;
        switch(*(value + i)) {
            case '-':
                if (i == 0) { neg = 1; } else { err = value + i; }
                break;
            case '+':
                if (i == 0) { neg = 0; } else { err = value + i; }
                break;
            case '0': break;
            case '1': out += 1; break;
            case '2': out += 2; break;
            case '3': out += 3; break;
            case '4': out += 4; break;
            case '5': out += 5; break;
            case '6': out += 6; break;
            case '7': out += 7; break;
            case '8': out += 8; break;
            case '9': out += 9; break;
            default: err = value + i;
        }
        if (err) {
            break;
        }
    }
    if (err && error != NULL) {
        *error = err;
    }
    return neg == 1 ? -out : out;
}

Pair * pair_new (char * name, size_t nlen, char * type, char * value, size_t vlen) {
    Pair * new = NULL;
    char * error = NULL;
    size_t i = 0;
    size_t j = 0;
    int hex = 0;

    new = calloc(1, sizeof(*new));
    if (new == NULL) { return NULL; }
    new->name = calloc(nlen + 1, sizeof(*new->name));
    if (new->name == NULL) { free(new); return NULL; }
    if (nlen > 0) {
        memcpy(new->name, name, nlen);
    }
    
    if (strncmp(type, "STR", 3) == 0) {
        new->type = STRING;
        new->sv = calloc(vlen + 1, sizeof(*value));
        if (new->sv == NULL) { free(new); return NULL; }
        new->length = vlen;
        memcpy(new->sv, value, vlen);
    } else if (strncmp(type, "INT", 3) == 0) {
        new->type = INTEGER;
        new->iv = toInt(value, vlen, &error);
        if (error) { free(new); return NULL; }
    } else if (strncmp(type, "FLO", 3) == 0) {
        new->type = FLOAT;
        new->fv = toFloat(value, vlen, &error);
        if (error) { free(new); return NULL; }
    } else if (strncmp(type, "BOO", 3) == 0) {
        new->type = BOOL;
        new->iv = toInt(value, 1, &error);
        if (error) { free(new); return NULL; }
    } else if (strncmp(type, "NUL", 3) == 0) {
        new->type = NUL;
    } else if (strncmp(type, "BST", 3) == 0) {
        new->type = BYTESTRING;
        new->sv = calloc(vlen, sizeof(*value));
        if (new->sv == NULL) { free(new); return NULL; }
        new->length = vlen;
        memcpy(new->sv, value, vlen);
    } else if (strncmp(type, "HEX", 3) == 0) {
        new->type = BYTESTRING;
        vlen = vlen % 2 != 0 ? vlen + 1 : vlen;
        new->length = vlen / 2;
        new->sv = calloc (new->length, sizeof(*value));
        if (new->sv == NULL) { free(new); return NULL; }
        for (i = 0; i < vlen; i++) {
            if (i > 0 && i % 2 == 0) { j++; }
            *(new->sv + j) <<= 4;
            switch (*(value + i)) {
                case '0': hex = 0; break;
                case '1': hex = 1; break;
                case '2': hex = 2; break;
                case '3': hex = 3; break;
                case '4': hex = 4; break;
                case '5': hex = 5; break;
                case '6': hex = 6; break;
                case '7': hex = 7; break;
                case '8': hex = 8; break;
                case '9': hex = 9; break;
                case 'a': case 'A': hex = 10; break;
                case 'b': case 'B': hex = 11; break;
                case 'c': case 'C': hex = 12; break;
                case 'd': case 'D': hex = 13; break;
                case 'e': case 'E': hex = 14; break;
                case 'f': case 'F': hex = 15; break;
            }
            *(new->sv + j) += hex;
        }
    } else if (strncmp(type, "OBJ", 3) == 0) {
        new->type = OBJECT;
        new->ov = object_new();
        if (new->ov == NULL) { free(new); return NULL; }
    }

    return new;
}
void pair_set_name (Pair * pair, const char * name) {
    char * newName = NULL;
    if (pair == NULL) { return; }
    if (name == NULL) { return; }
    newName = calloc(strlen(name) + 1, sizeof(*(pair->name)));
    if (newName == NULL) { return; }
    if (pair->name != NULL) { free(pair->name); }
    pair->name = newName;
    memcpy(pair->name, name, strlen(name));
}

void pair_free (Pair * pair) {
    if (pair == NULL) { return; }
    if (pair->type == OBJECT) {
        object_free(pair->ov);
    }
    if (pair->name != NULL) { free(pair->name); }
    if (pair->sv != NULL) { free(pair->sv); }
    free(pair);
}

int pair_get_value_at (Pair * pair, const char * path, Type * vtype, void ** value, size_t * vlen) {
    Pair * current = NULL;
    current = pair_at(pair, path);
    if (current) {
        return pair_get_value(current, vtype, value, vlen);
    }
    return 0;
}

Pair * pair_at(Pair * pair, const char * path) {
    char * sptr = NULL;
    char * str = strdup(path);
    char * value = NULL;
    Pair * found = NULL;
    size_t i = 0;

    value = strtok_r(str, ".", &sptr);
    while (value != NULL) {
        found = NULL;
        if (pair->type == OBJECT) {
            for (i = 0; i < pair->ov->count; i++) {
                if (*(pair->ov->list + i) != NULL && strcmp((*(pair->ov->list + i))->name, value) == 0) {
                    found = *(pair->ov->list + i);
                    break;
                }
            }
            if (found) { pair = found; } else { break; }
        }
        value = strtok_r(NULL, ".", &sptr);
    }
    free(str);
    return found;
}

int pair_get_value (Pair * pair, Type * vtype, void ** value, size_t * vlen) {
    if (value == NULL) { return 0; }
    if (value != NULL) { *value = NULL; }
    *vtype = NONE;
    if (vlen != NULL) { *vlen = 0; }
    if (pair == NULL) { return 0; }
    *vtype = pair->type;
    if (value != NULL) {
        switch (pair->type) {
            case OBJECT: *value = pair->ov; break;
            case BOOL:
            case INTEGER: *value = &(pair->iv); break;
            case BYTESTRING:
            case STRING: *value = pair->sv; break;
            case NUL: *value = NULL; break;
            case FLOAT: *value = &(pair->fv); break;
        }   
    }
    if (vlen != NULL) { *vlen = pair->length; }
    return 1;
}

void pair_print (Pair * pair, int level) {
    size_t i = 0;
    if (pair == NULL) { return; }
    for (i = 0; i < level * 3; i++) { printf(" "); }
    printf("%s:", pair->name);
    if (pair->type == OBJECT) {
        object_print(pair->ov, ++level);
    } else {
        switch(pair->type) {
            case INTEGER: printf("(int)<%ld>", pair->iv); break;
            case BOOL: printf(pair->iv ? "(bool)<true>" : "(bool)<false>"); break;
            case FLOAT: printf("(float)<%f>", pair->fv); break;
            case STRING: printf("(string)<%s>", pair->sv); break;
            case NUL: printf("(null)"); break;
            case BYTESTRING:
                printf("(bytestring)<");
                for (i = 0; i < pair->length; i++) {
                    printf("%x", *(pair->sv + i));
                }
                printf(">");
                break;
        }
        printf("\n");
    }
}

#define S_LEN 8
#define S_TYPE 3
#define O_TYPE 0
#define O_LNAME (S_TYPE + O_TYPE)
#define O_LVAL (O_LNAME + S_LEN)
#define O_CONTENT (O_LVAL + S_LEN)

Pair * proto_parse (Message * msg) {
    Pair * root = NULL;
    Pair * current = NULL;
    Pair * tmp = NULL;
    size_t i = 0;
    size_t lname = 0;
    size_t lval = 0;
    char * error = NULL;

    while (i < msg->len) {
        if (current != NULL && i >= current->ov->end) {
            if (current->ov->parent != NULL) {
                current = current->ov->parent;
            }
        }
        lname = toSize(msg->body + i + O_LNAME, S_LEN, &error);
        if (error) { break; }; error = NULL;
        lval = toSize(msg->body + i + O_LVAL, S_LEN, &error);
        if (error) { break; }; error = NULL;
        tmp = pair_new(msg->body + i + O_CONTENT, lname, msg->body + i + O_TYPE, msg->body + i + O_CONTENT + lname, lval);
        if (tmp == NULL) {
            break;
        }
        if (tmp->type != OBJECT) {
            pair_print(tmp, 0);
        }
        if (i == 0 && tmp->type != OBJECT) {
            current = pair_new("", 0, "OBJ", "", 0);
            if (current == NULL) {
                break;
            }
            current->ov->end = O_CONTENT + lname + lval;
        }
        if (i == 0) {
            if (current == NULL) {
                current = tmp;
            }
            root = current;
        } else {
            if (tmp->type == OBJECT) {
                tmp->ov->parent = current;
            }
            object_add_pair(current->ov, tmp);       
        }

        if (tmp->type == OBJECT) {
            i += O_CONTENT + lname;
            tmp->ov->end = i + lval;
            current = tmp;
        } else {
            i +=  O_CONTENT + lname + lval;
        }
    }

    return root;
}