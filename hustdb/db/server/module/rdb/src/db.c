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

#include <signal.h>
#include <ctype.h>

/*-----------------------------------------------------------------------------
 * C-level DB API
 *----------------------------------------------------------------------------*/

robj *lookupKey ( redisDb *db, robj *key )
{
    dictEntry *de = dictFind (db->dict, key->ptr);
    if ( de )
    {
        robj *val = dictGetVal (de);
        val->lru = LRU_CLOCK ();
        return val;
    }
    else
    {
        return NULL;
    }
}

robj *lookupKeyRead ( redisDb *db, robj *key )
{
    robj *val;

    if ( expireIfNeeded (db, key) == 1 )
    {
        /* Key expired. If we are in the context of a master, expireIfNeeded()
         * returns 0 only when the key does not exist at all, so it's save
         * to return NULL ASAP. */
        return NULL;
    }
    val = lookupKey (db, key);
    if ( val == NULL )
    {
        server.stat_keyspace_misses ++;
    }
    else
    {
        server.stat_keyspace_hits ++;
    }
    return val;
}

robj *lookupKeyWrite ( redisDb *db, robj *key )
{
    expireIfNeeded (db, key);
    return lookupKey (db, key);
}

/* Add the key to the DB. It's up to the caller to increment the reference
 * counter of the value if needed.
 *
 * The program is aborted if the key already exists. */
void dbAdd ( redisDb *db, robj *key, robj *val )
{
    sds copy = sdsdup (key->ptr);
    dictAdd (db->dict, copy, val);
}

/* Overwrite an existing key with a new value. Incrementing the reference
 * count of the new value is up to the caller.
 * This function does not modify the expire time of the existing key.
 *
 * The program is aborted if the key was not already present. */
void dbOverwrite ( redisDb *db, robj *key, robj *val )
{
    dictReplace (db->dict, key->ptr, val);
}

/* High level Set operation. This function can be used in order to set
 * a key, whatever it was existing or not, to a new object.
 *
 * 1) The ref count of the value object is incremented.
 * 2) clients WATCHing for the destination key notified.
 * 3) The expire time of the key is reset (the key is made persistent). */
void setKey ( redisDb *db, robj *key, robj *val )
{
    if ( lookupKeyWrite (db, key) == NULL )
    {
        dbAdd (db, key, val);
    }
    else
    {
        dbOverwrite (db, key, val);
    }
    incrRefCount (val);
    removeExpire (db, key);
}

int dbExists ( redisDb *db, robj *key )
{
    return dictFind (db->dict, key->ptr) != NULL;
}

/* Return a random key, in form of a Redis object.
 * If there are no keys, NULL is returned.
 *
 * The function makes sure to return keys not already expired. */
robj *dbRandomKey ( redisDb *db )
{
    dictEntry *de;

    while ( 1 )
    {
        sds key;
        robj *keyobj;

        de = dictGetRandomKey (db->dict);
        if ( de == NULL ) return NULL;

        key = dictGetKey (de);
        keyobj = createStringObject (key, sdslen (key));
        if ( dictFind (db->expires, key) )
        {
            if ( expireIfNeeded (db, keyobj) )
            {
                decrRefCount (keyobj);
                continue; /* search for another key. This expired. */
            }
        }
        return keyobj;
    }
}

/* Delete a key, value, and associated expiration entry if any, from the DB */
int dbDelete ( redisDb *db, robj *key )
{
    /* Deleting an entry from the expires dict will not free the sds of
     * the key, because it is shared with the main dictionary. */
    if ( dictSize (db->expires) > 0 ) dictDelete (db->expires, key->ptr);
    if ( dictDelete (db->dict, key->ptr) == DICT_OK )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* Prepare the string object stored at 'key' to be modified destructively
 * to implement commands like SETBIT or APPEND.
 *
 * An object is usually ready to be modified unless one of the two conditions
 * are true:
 *
 * 1) The object 'o' is shared (refcount > 1), we don't want to affect
 *    other users.
 * 2) The object encoding is not "RAW".
 *
 * If the object is found in one of the above conditions (or both) by the
 * function, an unshared / not-encoded copy of the string object is stored
 * at 'key' in the specified 'db'. Otherwise the object 'o' itself is
 * returned.
 *
 * USAGE:
 *
 * The object 'o' is what the caller already obtained by looking up 'key'
 * in 'db', the usage pattern looks like this:
 *
 * o = lookupKeyWrite(db,key);
 * if (checkType(c,o,OBJ_STRING)) return;
 * o = dbUnshareStringValue(db,key,o);
 *
 * At this point the caller is ready to modify the object, for example
 * using an sdscat() call to append some data, or anything else.
 */
robj *dbUnshareStringValue ( redisDb *db, robj *key, robj *o )
{
    if ( o->refcount != 1 || o->encoding != OBJ_ENCODING_RAW )
    {
        robj *decoded = getDecodedObject (o);
        o = createRawStringObject (decoded->ptr, sdslen (decoded->ptr));
        decrRefCount (decoded);
        dbOverwrite (db, key, o);
    }
    return o;
}

long long emptyDb ( void(callback ) ( void* ) )
{
    int j;
    long long removed = 0;

    for ( j = 0; j < server.dbnum; j ++ )
    {
        removed += dictSize (server.db[j].dict);
        dictEmpty (server.db[j].dict, callback);
        dictEmpty (server.db[j].expires, callback);
    }
    return removed;
}

int selectDb ( client *c, int id )
{
    if ( id < 0 || id >= server.dbnum )
        return C_ERR;
    c->db = & server.db[id];
    return C_OK;
}

/*-----------------------------------------------------------------------------
 * Type agnostic commands operating on the key space
 *----------------------------------------------------------------------------*/

int flushdbCommand ( client *c )
{
    dictEmpty (c->db->dict, NULL);
    dictEmpty (c->db->expires, NULL);
    return C_OK;
}

int flushallCommand ( client *c )
{
    emptyDb (NULL);
    return C_OK;
}

int delCommand ( client *c )
{
    int deleted = 0, j;

    for ( j = 1; j < c->argc; j ++ )
    {
        expireIfNeeded (c->db, c->argv[j]);
        if ( dbDelete (c->db, c->argv[j]) )
        {
            deleted ++;
        }
    }
    addReplyLongLong (c, deleted);
    return C_OK;
}

/* EXISTS key1 key2 ... key_N.
 * Return value is the number of keys existing. */
int existsCommand ( client *c )
{
    long long count = 0;
    int j;

    for ( j = 1; j < c->argc; j ++ )
    {
        expireIfNeeded (c->db, c->argv[j]);
        if ( dbExists (c->db, c->argv[j]) ) count ++;
    }
    addReplyLongLong (c, count);
    return C_OK;
}

int selectCommand ( client *c )
{
    long id;

    if ( getLongFromObject (c->argv[1], &id) != C_OK )
        return C_ERR;

    if ( selectDb (c, id) == C_ERR )
    {
        return C_ERR;
    }
    return C_OK;
}

int randomkeyCommand ( client *c )
{
    robj *key;

    if ( ( key = dbRandomKey (c->db) ) == NULL )
    {
        return C_ERR;
    }

    addReply (c, key);
    decrRefCount (key);
    return C_OK;
}

int keysCommand ( client *c )
{
    dictIterator *di;
    dictEntry *de;
    sds pattern = c->argv[1]->ptr;
    int plen = sdslen (pattern), allkeys;
    unsigned long numkeys = 0;

    di = dictGetSafeIterator (c->db->dict);
    allkeys = ( pattern[0] == '*' && pattern[1] == '\0' );
    while ( ( de = dictNext (di) ) != NULL )
    {
        sds key = dictGetKey (de);
        robj *keyobj;

        if ( allkeys || stringmatchlen (pattern, plen, key, sdslen (key), 0) )
        {
            keyobj = createStringObject (key, sdslen (key));
            if ( expireIfNeeded (c->db, keyobj) == 0 )
            {
                addReply (c, keyobj);
                numkeys ++;
            }
            decrRefCount (keyobj);
        }
    }
    dictReleaseIterator (di);
    return C_OK;
}

/* This callback is used by scanGenericCommand in order to collect elements
 * returned by the dictionary iterator into a list. */
void scanCallback ( void *privdata, const dictEntry *de )
{
    void **pd = ( void** ) privdata;
    list *keys = pd[0];
    robj *o = pd[1];
    robj *key, *val = NULL;

    if ( o == NULL )
    {
        sds sdskey = dictGetKey (de);
        key = createStringObject (sdskey, sdslen (sdskey));
    }
    else if ( o->type == OBJ_SET )
    {
        key = dictGetKey (de);
        incrRefCount (key);
    }
    else if ( o->type == OBJ_HASH )
    {
        key = dictGetKey (de);
        incrRefCount (key);
        val = dictGetVal (de);
        incrRefCount (val);
    }
    else if ( o->type == OBJ_ZSET )
    {
        key = dictGetKey (de);
        incrRefCount (key);
        val = createStringObjectFromLongDouble (*( double* ) dictGetVal (de), 0);
    }
    else
    {
        return;
    }

    listAddNodeTail (keys, key);
    if ( val ) listAddNodeTail (keys, val);
}

/* Try to parse a SCAN cursor stored at object 'o':
 * if the cursor is valid, store it as unsigned integer into *cursor and
 * returns C_OK. Otherwise return C_ERR and send an error to the
 * client. */
int parseScanCursor ( client *c, robj *o, unsigned long *cursor )
{
    char *eptr;

    /* Use strtoul() because we need an *unsigned* long, so
     * getLongLongFromObject() does not cover the whole cursor space. */
    errno = 0;
    *cursor = strtoul (o->ptr, &eptr, 10);
    if ( isspace (( ( char* ) o->ptr )[0]) || eptr[0] != '\0' || errno == ERANGE )
    {
        return C_ERR;
    }
    return C_OK;
}

/* This command implements SCAN, HSCAN and SSCAN commands.
 * If object 'o' is passed, then it must be a Hash or Set object, otherwise
 * if 'o' is NULL the command will operate on the dictionary associated with
 * the current database.
 *
 * When 'o' is not NULL the function assumes that the first argument in
 * the client arguments vector is a key so it skips it before iterating
 * in order to parse options.
 *
 * In the case of a Hash object the function returns both the field and value
 * of every element on the Hash. */
void scanGenericCommand ( client *c, robj *o, unsigned long cursor )
{
    int i, j;
    list *keys = listCreate ();
    listNode *node, *nextnode;
    long count = 10;
    sds pat = NULL;
    int patlen = 0, use_pattern = 0;
    dict *ht;


    /* Set i to the first option argument. The previous one is the cursor. */
    i = ( o == NULL ) ? 2 : 3; /* Skip the key argument if needed. */

    /* Step 1: Parse options. */
    while ( i < c->argc )
    {
        j = c->argc - i;
        if ( ! strcasecmp (c->argv[i]->ptr, "count") && j >= 2 )
        {
            if ( getLongFromObject (c->argv[i + 1], &count)
                 != C_OK )
            {
                goto cleanup;
            }

            if ( count < 1 )
            {
                goto cleanup;
            }

            i += 2;
        }
        else if ( ! strcasecmp (c->argv[i]->ptr, "match") && j >= 2 )
        {
            pat = c->argv[i + 1]->ptr;
            patlen = sdslen (pat);

            /* The pattern always matches if it is exactly "*", so it is
             * equivalent to disabling it. */
            use_pattern = ! ( pat[0] == '*' && patlen == 1 );

            i += 2;
        }
        else
        {
            goto cleanup;
        }
    }

    /* Step 2: Iterate the collection.
     *
     * Note that if the object is encoded with a ziplist, intset, or any other
     * representation that is not a hash table, we are sure that it is also
     * composed of a small number of elements. So to avoid taking state we
     * just return everything inside the object in a single call, setting the
     * cursor to zero to signal the end of the iteration. */

    /* Handle the case of a hash table. */
    ht = NULL;
    if ( o == NULL )
    {
        ht = c->db->dict;
    }
    else if ( o->type == OBJ_SET && o->encoding == OBJ_ENCODING_HT )
    {
        ht = o->ptr;
    }
    else if ( o->type == OBJ_HASH && o->encoding == OBJ_ENCODING_HT )
    {
        ht = o->ptr;
        count *= 2; /* We return key / value for this type. */
    }
    else if ( o->type == OBJ_ZSET && o->encoding == OBJ_ENCODING_SKIPLIST )
    {
        zset *zs = o->ptr;
        ht = zs->dict;
        count *= 2; /* We return key / value for this type. */
    }

    if ( ht )
    {
        void *privdata[2];
        /* We set the max number of iterations to ten times the specified
         * COUNT, so if the hash table is in a pathological state (very
         * sparsely populated) we avoid to block too much time at the cost
         * of returning no or very few elements. */
        long maxiterations = count * 10;

        /* We pass two pointers to the callback: the list to which it will
         * add new elements, and the object containing the dictionary so that
         * it is possible to fetch more data in a type-dependent way. */
        privdata[0] = keys;
        privdata[1] = o;
        do
        {
            cursor = dictScan (ht, cursor, scanCallback, privdata);
        }
        while ( cursor &&
                maxiterations -- &&
                listLength (keys) < ( unsigned long ) count );
    }
    else if ( o->type == OBJ_SET )
    {
        int pos = 0;
        int64_t ll;

        while ( intsetGet (o->ptr, pos ++, &ll) )
            listAddNodeTail (keys, createStringObjectFromLongLong (ll));
        cursor = 0;
    }
    else if ( o->type == OBJ_HASH || o->type == OBJ_ZSET )
    {
        unsigned char *p = ziplistIndex (o->ptr, 0);
        unsigned char *vstr;
        unsigned int vlen;
        long long vll;

        while ( p )
        {
            ziplistGet (p, &vstr, &vlen, &vll);
            listAddNodeTail (keys,
                             ( vstr != NULL ) ? createStringObject (( char* ) vstr, vlen) :
                             createStringObjectFromLongLong (vll));
            p = ziplistNext (o->ptr, p);
        }
        cursor = 0;
    }
    else
    {
        return;
    }

    /* Step 3: Filter elements. */
    node = listFirst (keys);
    while ( node )
    {
        robj *kobj = listNodeValue (node);
        nextnode = listNextNode (node);
        int filter = 0;

        /* Filter element if it does not match the pattern. */
        if ( ! filter && use_pattern )
        {
            if ( sdsEncodedObject (kobj) )
            {
                if ( ! stringmatchlen (pat, patlen, kobj->ptr, sdslen (kobj->ptr), 0) )
                    filter = 1;
            }
            else
            {
                char buf[LONG_STR_SIZE];
                int len;

                len = ll2string (buf, sizeof (buf ), ( long ) kobj->ptr);
                if ( ! stringmatchlen (pat, patlen, buf, len, 0) ) filter = 1;
            }
        }

        /* Filter element if it is an expired key. */
        if ( ! filter && o == NULL && expireIfNeeded (c->db, kobj) ) filter = 1;

        /* Remove the element and its associted value if needed. */
        if ( filter )
        {
            decrRefCount (kobj);
            listDelNode (keys, node);
        }

        /* If this is a hash or a sorted set, we have a flat list of
         * key-value elements, so if this element was filtered, remove the
         * value, or skip it if it was not filtered: we only match keys. */
        if ( o && ( o->type == OBJ_ZSET || o->type == OBJ_HASH ) )
        {
            node = nextnode;
            nextnode = listNextNode (node);
            if ( filter )
            {
                kobj = listNodeValue (node);
                decrRefCount (kobj);
                listDelNode (keys, node);
            }
        }
        node = nextnode;
    }

    /* Step 4: Reply to the client. */
    addReplyLongLong (c, cursor);

    while ( ( node = listFirst (keys) ) != NULL )
    {
        robj *kobj = listNodeValue (node);
        addReply (c, kobj);
        decrRefCount (kobj);
        listDelNode (keys, node);
    }

cleanup:
    listSetFreeMethod (keys, decrRefCountVoid);
    listRelease (keys);
}

/* The SCAN command completely relies on scanGenericCommand. */
int scanCommand ( client *c )
{
    unsigned long cursor;
    if ( parseScanCursor (c, c->argv[1], &cursor) == C_ERR ) return C_ERR;
    scanGenericCommand (c, NULL, cursor);
    return C_OK;
}

int dbsizeCommand ( client *c )
{
    addReplyLongLong (c, dictSize (c->db->dict));
    return C_OK;
}

int typeCommand ( client *c )
{
    robj *o;
    char *type;

    o = lookupKeyRead (c->db, c->argv[1]);
    if ( o == NULL )
    {
        type = "none";
    }
    else
    {
        switch ( o->type )
        {
            case OBJ_STRING: type = "string";
                break;
            case OBJ_LIST: type = "list";
                break;
            case OBJ_SET: type = "set";
                break;
            case OBJ_ZSET: type = "zset";
                break;
            case OBJ_HASH: type = "hash";
                break;
            default: type = "unknown";
                break;
        }
    }
    addReplyString (c, type, strlen (type));
    return C_OK;
}

int renameGenericCommand ( client *c, int nx )
{
    robj *o;
    long long expire;
    int samekey = 0;

    /* When source and dest key is the same, no operation is performed,
     * if the key exists, however we still return an error on unexisting key. */
    if ( sdscmp (c->argv[1]->ptr, c->argv[2]->ptr) == 0 ) samekey = 1;

    if ( ( o = lookupKeyWrite (c->db, c->argv[1]) ) == NULL )
        return C_ERR;

    if ( samekey )
    {
        return C_OK;
    }

    incrRefCount (o);
    expire = getExpire (c->db, c->argv[1]);
    if ( lookupKeyWrite (c->db, c->argv[2]) != NULL )
    {
        if ( nx )
        {
            decrRefCount (o);
            return C_OK;
        }
        /* Overwrite: delete the old key before creating the new one
         * with the same name. */
        dbDelete (c->db, c->argv[2]);
    }
    dbAdd (c->db, c->argv[2], o);
    if ( expire != - 1 ) setExpire (c->db, c->argv[2], expire);
    dbDelete (c->db, c->argv[1]);
    return C_OK;
}

int renameCommand ( client *c )
{
    return renameGenericCommand (c, 0);
}

int renamenxCommand ( client *c )
{
    return renameGenericCommand (c, 1);
}

int moveCommand ( client *c )
{
    robj *o;
    redisDb *src, *dst;
    int srcid;
    long long dbid, expire;

    /* Obtain source and target DB pointers */
    src = c->db;
    srcid = c->db->id;

    if ( getLongLongFromObject (c->argv[2], &dbid) == C_ERR ||
         dbid < INT_MIN || dbid > INT_MAX ||
         selectDb (c, dbid) == C_ERR )
    {
        return C_ERR;
    }
    dst = c->db;
    selectDb (c, srcid); /* Back to the source DB */

    /* If the user is moving using as target the same
     * DB as the source DB it is probably an error. */
    if ( src == dst )
    {
        return C_ERR;
    }

    /* Check if the element exists and get a reference */
    o = lookupKeyWrite (c->db, c->argv[1]);
    if ( ! o )
    {
        return C_ERR;
    }
    expire = getExpire (c->db, c->argv[1]);

    /* Return zero if the key already exists in the target DB */
    if ( lookupKeyWrite (dst, c->argv[1]) != NULL )
    {
        return C_ERR;
    }
    dbAdd (dst, c->argv[1], o);
    if ( expire != - 1 ) setExpire (dst, c->argv[1], expire);
    incrRefCount (o);

    /* OK! key moved, free the entry in the source DB */
    dbDelete (src, c->argv[1]);
    return C_OK;
}

/*-----------------------------------------------------------------------------
 * Expires API
 *----------------------------------------------------------------------------*/

int removeExpire ( redisDb *db, robj *key )
{
    /* An expire may only be removed if there is a corresponding entry in the
     * main dict. Otherwise, the key will never be freed. */
    if ( dictFind (db->dict, key->ptr) == NULL )
    {
        return C_ERR;
    }
    return dictDelete (db->expires, key->ptr) == DICT_OK;
}

void setExpire ( redisDb *db, robj *key, long long when )
{
    dictEntry *kde, *de;

    /* Reuse the sds from the main dict in the expire dict */
    kde = dictFind (db->dict, key->ptr);
    de = dictReplaceRaw (db->expires, dictGetKey (kde));
    dictSetSignedIntegerVal (de, when);
}

/* Return the expire time of the specified key, or -1 if no expire
 * is associated with this key (i.e. the key is non volatile) */
long long getExpire ( redisDb *db, robj *key )
{
    dictEntry *de;

    /* No expire? return ASAP */
    if ( dictSize (db->expires) == 0 ||
         ( de = dictFind (db->expires, key->ptr) ) == NULL ) return - 1;

    /* The entry was found in the expire dict, this means it should also
     * be present in the main dict (safety check). */
    if ( dictFind (db->dict, key->ptr) == NULL )
    {
        return C_ERR;
    }
    return dictGetSignedIntegerVal (de);
}

int expireIfNeeded ( redisDb *db, robj *key )
{
    mstime_t when = getExpire (db, key);
    mstime_t now;

    if ( when < 0 ) return 0; /* No expire for this key */

    now = mstime ();

    /* Return when this key has not expired */
    if ( now <= when ) return 0;

    /* Delete the key */
    server.stat_expiredkeys ++;
    return dbDelete (db, key);
}

/*-----------------------------------------------------------------------------
 * Expires Commands
 *----------------------------------------------------------------------------*/

/* This is the generic command implementation for EXPIRE, PEXPIRE, EXPIREAT
 * and PEXPIREAT. Because the commad second argument may be relative or absolute
 * the "basetime" argument is used to signal what the base time is (either 0
 * for *AT variants of the command, or the current time for relative expires).
 *
 * unit is either UNIT_SECONDS or UNIT_MILLISECONDS, and is only used for
 * the argv[2] parameter. The basetime is always specified in milliseconds. */
int expireGenericCommand ( client *c, long long basetime, int unit )
{
    robj *key = c->argv[1], *param = c->argv[2];
    long long when; /* unix time in milliseconds when the key will expire. */

    if ( getLongLongFromObject (param, &when) != C_OK )
        return C_ERR;

    if ( unit == UNIT_SECONDS ) when *= 1000;
    when += basetime;

    /* No key, return zero. */
    if ( lookupKeyWrite (c->db, key) == NULL )
    {
        return C_ERR;
    }

    /* EXPIRE with negative TTL, or EXPIREAT with a timestamp into the past
     * should never be executed as a DEL when load the AOF or in the context
     * of a slave instance.
     *
     * Instead we take the other branch of the IF statement setting an expire
     * (possibly in the past) and wait for an explicit DEL from the master. */
    if ( when <= mstime () )
    {
        return C_ERR;
    }
    else
    {
        setExpire (c->db, key, when);
        return C_OK;
    }
}

int expireCommand ( client *c )
{
    return expireGenericCommand (c, mstime (), UNIT_SECONDS);
}

int expireatCommand ( client *c )
{
    return expireGenericCommand (c, 0, UNIT_SECONDS);
}

int pexpireCommand ( client *c )
{
    return expireGenericCommand (c, mstime (), UNIT_MILLISECONDS);
}

int pexpireatCommand ( client *c )
{
    return expireGenericCommand (c, 0, UNIT_MILLISECONDS);
}

int ttlGenericCommand ( client *c, int output_ms )
{
    long long expire, ttl = - 1;

    /* If the key does not exist at all, return -2 */
    if ( lookupKeyRead (c->db, c->argv[1]) == NULL )
    {
        return C_ERR;
    }
    /* The key exists. Return -1 if it has no expire, or the actual
     * TTL value otherwise. */
    expire = getExpire (c->db, c->argv[1]);
    if ( expire != - 1 )
    {
        ttl = expire - mstime ();
        if ( ttl < 0 ) ttl = 0;
    }
    if ( ttl == - 1 )
    {
        addReplyLongLong (c, - 1);
    }
    else
    {
        addReplyLongLong (c, output_ms ? ttl : ( ( ttl + 500 ) / 1000 ));
    }
    return C_OK;
}

int ttlCommand ( client *c )
{
    return ttlGenericCommand (c, 0);
}

int pttlCommand ( client *c )
{
    return ttlGenericCommand (c, 1);
}

int persistCommand ( client *c )
{
    dictEntry *de;

    de = dictFind (c->db->dict, c->argv[1]->ptr);
    if ( de == NULL )
    {
        return C_ERR;
    }
    else
    {
        if ( removeExpire (c->db, c->argv[1]) )
        {
            return C_OK;
        }
        else
        {
            return C_ERR;
        }
    }
}

/* -----------------------------------------------------------------------------
 * API to get key arguments from commands
 * ---------------------------------------------------------------------------*/

/* The base case is to use the keys position as given in the command table
 * (firstkey, lastkey, step). */
int *getKeysUsingCommandTable ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    int j, i = 0, last, *keys;
    UNUSED (argv);

    if ( cmd->firstkey == 0 )
    {
        *numkeys = 0;
        return NULL;
    }
    last = cmd->lastkey;
    if ( last < 0 ) last = argc + last;
    keys = zmalloc (sizeof (int )*( ( last - cmd->firstkey ) + 1 ));
    for ( j = cmd->firstkey; j <= last; j += cmd->keystep )
    {
        keys[i ++] = j;
    }
    *numkeys = i;
    return keys;
}

/* Return all the arguments that are keys in the command passed via argc / argv.
 *
 * The command returns the positions of all the key arguments inside the array,
 * so the actual return value is an heap allocated array of integers. The
 * length of the array is returned by reference into *numkeys.
 *
 * 'cmd' must be point to the corresponding entry into the redisCommand
 * table, according to the command name in argv[0].
 *
 * This function uses the command table if a command-specific helper function
 * is not required, otherwise it calls the command-specific function. */
int *getKeysFromCommand ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    if ( cmd->getkeys_proc )
    {
        return cmd->getkeys_proc (cmd, argv, argc, numkeys);
    }
    else
    {
        return getKeysUsingCommandTable (cmd, argv, argc, numkeys);
    }
}

/* Free the result of getKeysFromCommand. */
void getKeysFreeResult ( int *result )
{
    zfree (result);
}

/* Helper function to extract keys from following commands:
 * ZUNIONSTORE <destkey> <num-keys> <key> <key> ... <key> <options>
 * ZINTERSTORE <destkey> <num-keys> <key> <key> ... <key> <options> */
int *zunionInterGetKeys ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    int i, num, *keys;
    UNUSED (cmd);

    num = atoi (argv[2]->ptr);
    /* Sanity check. Don't return any key if the command is going to
     * reply with syntax error. */
    if ( num > ( argc - 3 ) )
    {
        *numkeys = 0;
        return NULL;
    }

    /* Keys in z{union,inter}store come from two places:
     * argv[1] = storage key,
     * argv[3...n] = keys to intersect */
    keys = zmalloc (sizeof (int )*( num + 1 ));

    /* Add all key positions for argv[3...n] to keys[] */
    for ( i = 0; i < num; i ++ ) keys[i] = 3 + i;

    /* Finally add the argv[1] key position (the storage key target). */
    keys[num] = 1;
    *numkeys = num + 1; /* Total keys = {union,inter} keys + storage key */
    return keys;
}

/* Helper function to extract keys from the following commands:
 * EVAL <script> <num-keys> <key> <key> ... <key> [more stuff]
 * EVALSHA <script> <num-keys> <key> <key> ... <key> [more stuff] */
int *evalGetKeys ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    int i, num, *keys;
    UNUSED (cmd);

    num = atoi (argv[2]->ptr);
    /* Sanity check. Don't return any key if the command is going to
     * reply with syntax error. */
    if ( num > ( argc - 3 ) )
    {
        *numkeys = 0;
        return NULL;
    }

    keys = zmalloc (sizeof (int )*num);
    *numkeys = num;

    /* Add all key positions for argv[3...n] to keys[] */
    for ( i = 0; i < num; i ++ ) keys[i] = 3 + i;

    return keys;
}

/* Helper function to extract keys from the SORT command.
 *
 * SORT <sort-key> ... STORE <store-key> ...
 *
 * The first argument of SORT is always a key, however a list of options
 * follow in SQL-alike style. Here we parse just the minimum in order to
 * correctly identify keys in the "STORE" option. */
int *sortGetKeys ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    int i, j, num, *keys, found_store = 0;
    UNUSED (cmd);

    num = 0;
    keys = zmalloc (sizeof (int )*2); /* Alloc 2 places for the worst case. */

    keys[num ++] = 1; /* <sort-key> is always present. */

    /* Search for STORE option. By default we consider options to don't
     * have arguments, so if we find an unknown option name we scan the
     * next. However there are options with 1 or 2 arguments, so we
     * provide a list here in order to skip the right number of args. */
    struct
    {
        char *name;
        int skip;
    } skiplist[] = {
        {"limit", 2 },
        {"get", 1 },
        {"by", 1 },
        {NULL, 0 } /* End of elements. */
    };

    for ( i = 2; i < argc; i ++ )
    {
        for ( j = 0; skiplist[j].name != NULL; j ++ )
        {
            if ( ! strcasecmp (argv[i]->ptr, skiplist[j].name) )
            {
                i += skiplist[j].skip;
                break;
            }
            else if ( ! strcasecmp (argv[i]->ptr, "store") && i + 1 < argc )
            {
                /* Note: we don't increment "num" here and continue the loop
                 * to be sure to process the *last* "STORE" option if multiple
                 * ones are provided. This is same behavior as SORT. */
                found_store = 1;
                keys[num] = i + 1; /* <store-key> */
                break;
            }
        }
    }
    *numkeys = num + found_store;
    return keys;
}

int *migrateGetKeys ( struct redisCommand *cmd, robj **argv, int argc, int *numkeys )
{
    int i, num, first, *keys;
    UNUSED (cmd);

    /* Assume the obvious form. */
    first = 3;
    num = 1;

    /* But check for the extended one with the KEYS option. */
    if ( argc > 6 )
    {
        for ( i = 6; i < argc; i ++ )
        {
            if ( ! strcasecmp (argv[i]->ptr, "keys") &&
                 sdslen (argv[3]->ptr) == 0 )
            {
                first = i + 1;
                num = argc - first;
                break;
            }
        }
    }

    keys = zmalloc (sizeof (int )*num);
    for ( i = 0; i < num; i ++ ) keys[i] = first + i;
    *numkeys = num;
    return keys;
}
