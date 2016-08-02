/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "../lib/server.h"
#include <math.h> /* isnan(), isinf() */

/*-----------------------------------------------------------------------------
 * String Commands
 *----------------------------------------------------------------------------*/

static int checkStringLength(client *c, long long size) {
    if (size > 512*1024*1024) {
        return C_ERR;
    }
    return C_OK;
}

/* The setGenericCommand() function implements the SET operation with different
 * options and variants. This function is called in order to implement the
 * following commands: SET, SETEX, PSETEX, SETNX.
 *
 * 'flags' changes the behavior of the command (NX or XX, see belove).
 *
 * 'expire' represents an expire to set in form of a Redis object as passed
 * by the user. It is interpreted according to the specified 'unit'.
 *
 * 'ok_reply' and 'abort_reply' is what the function will reply to the client
 * if the operation is performed, or when it is not because of NX or
 * XX flags.
 *
 * If ok_reply is NULL "+OK" is used.
 * If abort_reply is NULL, "$-1" is used. */

#define OBJ_SET_NO_FLAGS 0
#define OBJ_SET_NX (1<<0)     /* Set if key not exists. */
#define OBJ_SET_XX (1<<1)     /* Set if key exists. */
#define OBJ_SET_EX (1<<2)     /* Set if time in seconds is given */
#define OBJ_SET_PX (1<<3)     /* Set if time in ms in given */

int setGenericCommand(client *c, int flags, robj *key, robj *val, robj *expire, int unit) {
    long long milliseconds = 0; /* initialized to avoid any harmness warning */

    if (expire) {
        if (getLongLongFromObject(expire, &milliseconds) != C_OK)
            return C_ERR;
        if (milliseconds <= 0) {
            return C_ERR;
        }
        if (unit == UNIT_SECONDS) milliseconds *= 1000;
    }

    if ((flags & OBJ_SET_NX && lookupKeyWrite(c->db,key) != NULL) ||
        (flags & OBJ_SET_XX && lookupKeyWrite(c->db,key) == NULL))
    {
        return C_ERR;
    }
    setKey(c->db,key,val);
    if (expire) setExpire(c->db,key,mstime()+milliseconds);
    return C_OK;
}

/* SET key value [NX] [XX] [EX <seconds>] [PX <milliseconds>] */
int setCommand(client *c) {
    int j;
    robj *expire = NULL;
    int unit = UNIT_SECONDS;
    int flags = OBJ_SET_NO_FLAGS;

    for (j = 3; j < c->argc; j++) {
        char *a = c->argv[j]->ptr;
        robj *next = (j == c->argc-1) ? NULL : c->argv[j+1];

        if ((a[0] == 'n' || a[0] == 'N') &&
            (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
            !(flags & OBJ_SET_XX))
        {
            flags |= OBJ_SET_NX;
        } else if ((a[0] == 'x' || a[0] == 'X') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_NX))
        {
            flags |= OBJ_SET_XX;
        } else if ((a[0] == 'e' || a[0] == 'E') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_PX) && next)
        {
            flags |= OBJ_SET_EX;
            unit = UNIT_SECONDS;
            expire = next;
            j++;
        } else if ((a[0] == 'p' || a[0] == 'P') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_EX) && next)
        {
            flags |= OBJ_SET_PX;
            unit = UNIT_MILLISECONDS;
            expire = next;
            j++;
        } else {
            return C_ERR;
        }
    }

    c->argv[2] = tryObjectEncoding(c->argv[2]);
    return setGenericCommand(c,flags,c->argv[1],c->argv[2],expire,unit);
}

int setnxCommand(client *c) {
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    return setGenericCommand(c,OBJ_SET_NX,c->argv[1],c->argv[2],NULL,0);
}

int setexCommand(client *c) {
    c->argv[3] = tryObjectEncoding(c->argv[3]);
    return setGenericCommand(c,OBJ_SET_NO_FLAGS,c->argv[1],c->argv[3],c->argv[2],UNIT_SECONDS);
}

int psetexCommand(client *c) {
    c->argv[3] = tryObjectEncoding(c->argv[3]);
    return setGenericCommand(c,OBJ_SET_NO_FLAGS,c->argv[1],c->argv[3],c->argv[2],UNIT_MILLISECONDS);
}

int getGenericCommand(client *c) {
    robj *o;

    if ((o = lookupKeyRead(c->db,c->argv[1])) == NULL){
        addReplyNULL(c);
        return C_OK;
    }

    if (o->type != OBJ_STRING) {
        return C_ERR;
    } else {
        addReply(c,o);
        return C_OK;
    }
}

int getCommand(client *c) {
    return getGenericCommand(c);
}

int getsetCommand(client *c) {
    if (getGenericCommand(c) == C_ERR) return C_ERR;
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setKey(c->db,c->argv[1],c->argv[2]);
    return C_OK;
}

int setrangeCommand(client *c) {
    robj *o;
    long offset;
    sds value = c->argv[3]->ptr;

    if (getLongFromObject(c->argv[2],&offset) != C_OK)
        return C_ERR;

    if (offset < 0) {
        return C_ERR;
    }

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o == NULL) {
        /* Return 0 when setting nothing on a non-existing string */
        if (sdslen(value) == 0) {
            addReplyLongLong(c, 0);
            return C_OK;
        }

        /* Return when the resulting string exceeds allowed size */
        if (checkStringLength(c,offset+sdslen(value)) != C_OK)
            return C_ERR;

        o = createObject(OBJ_STRING,sdsnewlen(NULL, offset+sdslen(value)));
        dbAdd(c->db,c->argv[1],o);
    } else {
        size_t olen;

        /* Key exists, check type */
        if (checkType(c,o,OBJ_STRING))
            return C_ERR;

        /* Return existing string length when setting nothing */
        olen = stringObjectLen(o);
        if (sdslen(value) == 0) {
            addReplyLongLong(c,olen);
            return C_OK;
        }

        /* Return when the resulting string exceeds allowed size */
        if (checkStringLength(c,offset+sdslen(value)) != C_OK)
            return C_ERR;

        /* Create a copy when the object is shared or encoded. */
        o = dbUnshareStringValue(c->db,c->argv[1],o);
    }

    if (sdslen(value) > 0) {
        o->ptr = sdsgrowzero(o->ptr,offset+sdslen(value));
        memcpy((char*)o->ptr+offset,value,sdslen(value));
    }
    addReplyLongLong(c,sdslen(o->ptr));
    return C_OK;
}

int getrangeCommand(client *c) {
    robj *o;
    long long start, end;
    char *str, llbuf[32];
    size_t strlen;

    if (getLongLongFromObject(c->argv[2],&start) != C_OK)
        return C_ERR;
    if (getLongLongFromObject(c->argv[3],&end) != C_OK)
        return C_ERR;
    if ((o = lookupKeyRead(c->db,c->argv[1])) == NULL ||
        checkType(c,o,OBJ_STRING)) return C_ERR;

    if (o->encoding == OBJ_ENCODING_INT) {
        str = llbuf;
        strlen = ll2string(llbuf,sizeof(llbuf),(long)o->ptr);
    } else {
        str = o->ptr;
        strlen = sdslen(str);
    }

    /* Convert negative indexes */
    if (start < 0) start = strlen+start;
    if (end < 0) end = strlen+end;
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if ((unsigned long long)end >= strlen) end = strlen-1;

    /* Precondition: end >= 0 && end < strlen, so the only condition where
     * nothing can be returned is: start > end. */
    if (start > end || strlen == 0) {
        return C_ERR;
    } else {
        addReplyString(c,(char*)str+start,end-start+1);
    }
    return C_OK;
}

int mgetCommand(client *c) {
    int j;

    for (j = 1; j < c->argc; j++) {
        robj *o = lookupKeyRead(c->db,c->argv[j]);
        if (o == NULL) {
            addReplyNULL(c);
        } else {
            if (o->type != OBJ_STRING) {
                addReplyNULL(c);
            } else {
                addReply(c,o);
            }
        }
    }
    return C_OK;
}

int msetGenericCommand(client *c, int nx) {
    int j, busykeys = 0;

    if ((c->argc % 2) == 0) {
        return C_ERR;
    }
    /* Handle the NX flag. The MSETNX semantic is to return zero and don't
     * set nothing at all if at least one already key exists. */
    if (nx) {
        for (j = 1; j < c->argc; j += 2) {
            if (lookupKeyWrite(c->db,c->argv[j]) != NULL) {
                busykeys++;
            }
        }
        if (busykeys) {
            return C_ERR;
        }
    }

    for (j = 1; j < c->argc; j += 2) {
        c->argv[j+1] = tryObjectEncoding(c->argv[j+1]);
        setKey(c->db,c->argv[j],c->argv[j+1]);
    }
    return C_OK;
}

int msetCommand(client *c) {
    return msetGenericCommand(c,0);
}

int msetnxCommand(client *c) {
    return msetGenericCommand(c,1);
}

int incrDecrCommand(client *c, long long incr) {
    long long value, oldvalue;
    robj *o, *new;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o != NULL && checkType(c,o,OBJ_STRING)) return C_ERR ;
    if (getLongLongFromObject(o,&value) != C_OK) return C_ERR;

    oldvalue = value;
    if ((incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)) ||
        (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))) {
        return C_ERR;
    }
    value += incr;

    if (o && o->refcount == 1 && o->encoding == OBJ_ENCODING_INT &&
        (value < 0 || value >= OBJ_SHARED_INTEGERS) &&
        value >= LONG_MIN && value <= LONG_MAX)
    {
        new = o;
        o->ptr = (void*)((long)value);
    } else {
        new = createStringObjectFromLongLong(value);
        if (o) {
            dbOverwrite(c->db,c->argv[1],new);
        } else {
            dbAdd(c->db,c->argv[1],new);
        }
    }
    addReply(c,new);
    return C_OK;
}

int incrCommand(client *c) {
    return incrDecrCommand(c,1);
}

int decrCommand(client *c) {
    return incrDecrCommand(c,-1);
}

int incrbyCommand(client *c) {
    long long incr;

    if (getLongLongFromObject(c->argv[2], &incr) != C_OK) return C_ERR;
    return incrDecrCommand(c,incr);
}

int decrbyCommand(client *c) {
    long long incr;

    if (getLongLongFromObject(c->argv[2], &incr) != C_OK) return C_ERR;
    return incrDecrCommand(c,-incr);
}

int incrbyfloatCommand(client *c) {
    long double incr, value;
    robj *o, *new;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o != NULL && checkType(c,o,OBJ_STRING)) return C_ERR;
    if (getLongDoubleFromObject(o,&value) != C_OK ||
        getLongDoubleFromObject(c->argv[2],&incr) != C_OK)
        return C_ERR;

    value += incr;

    new = createStringObjectFromLongDouble(value,1);
    if (o)
        dbOverwrite(c->db,c->argv[1],new);
    else
        dbAdd(c->db,c->argv[1],new);
    addReply(c,new);

    return C_OK;
}

int appendCommand(client *c) {
    size_t totlen;
    robj *o, *append;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o == NULL) {
        /* Create the key */
        c->argv[2] = tryObjectEncoding(c->argv[2]);
        dbAdd(c->db,c->argv[1],c->argv[2]);
        incrRefCount(c->argv[2]);
        totlen = stringObjectLen(c->argv[2]);
    } else {
        /* Key exists, check type */
        if (checkType(c,o,OBJ_STRING))
            return C_ERR;

        /* "append" is an argument, so always an sds */
        append = c->argv[2];
        totlen = stringObjectLen(o)+sdslen(append->ptr);
        if (checkStringLength(c,totlen) != C_OK)
            return C_ERR;

        /* Append the value */
        o = dbUnshareStringValue(c->db,c->argv[1],o);
        o->ptr = sdscatlen(o->ptr,append->ptr,sdslen(append->ptr));
        totlen = sdslen(o->ptr);
    }
    addReplyLongLong(c,totlen);
    return C_OK;
}

int strlenCommand(client *c) {
    robj *o;
    if ((o = lookupKeyRead(c->db,c->argv[1])) == NULL ||
        checkType(c,o,OBJ_STRING)) return C_ERR;
    addReplyLongLong(c,stringObjectLen(o));
    return C_OK;
}
