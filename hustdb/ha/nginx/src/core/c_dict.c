#include "c_dict.h"
#include <stdlib.h>
#include <string.h>

struct c_dict_node_t
{
    unsigned hash;
    void *value;
    c_dict_node_t *next;
};

static unsigned c_dict_hash(const char *str)
{
    unsigned hash = 5381;
    while (*str)
    {
        hash = ((hash << 5) + hash) ^ *str++;
    }
    return hash;
}

static c_dict_node_t *c_dict_newnode(const char *key, void *value, int vsize)
{
    c_dict_node_t *node;
    int ksize = strlen(key) + 1;
    int voffset = ksize + ((sizeof(void*) - ksize) % sizeof(void*));
    node = malloc(sizeof(*node) + voffset + vsize);
    if (!node)
    {
        return NULL;
    }
    memcpy(node + 1, key, ksize);
    node->hash = c_dict_hash(key);
    node->value = ((char*) (node + 1)) + voffset;
    memcpy(node->value, value, vsize);
    return node;
}

static int c_dict_bucketidx(c_dict_base_t *dict, unsigned hash)
{
    /* If the implementation is changed to allow a non-power-of-2 bucket count,
     * the line below should be changed to use mod instead of AND */
    return hash & (dict->nbuckets - 1);
}

static void c_dict_addnode(c_dict_base_t *dict, c_dict_node_t *node)
{
    int n = c_dict_bucketidx(dict, node->hash);
    node->next = dict->buckets[n];
    dict->buckets[n] = node;
}

static int c_dict_resize(c_dict_base_t *dict, int nbuckets)
{
    c_dict_node_t *nodes, *node, *next;
    c_dict_node_t **buckets;
    int i;
    /* Chain all nodes together */
    nodes = NULL;
    i = dict->nbuckets;
    while (i--)
    {
        node = (dict->buckets)[i];
        while (node)
        {
            next = node->next;
            node->next = nodes;
            nodes = node;
            node = next;
        }
    }
    /* Reset buckets */
    buckets = realloc(dict->buckets, sizeof(*dict->buckets) * nbuckets);
    if (buckets != NULL)
    {
        dict->buckets = buckets;
        dict->nbuckets = nbuckets;
    }
    if (dict->buckets)
    {
        memset(dict->buckets, 0, sizeof(*dict->buckets) * dict->nbuckets);
        /* Re-add nodes to buckets */
        node = nodes;
        while (node)
        {
            next = node->next;
            c_dict_addnode(dict, node);
            node = next;
        }
    }
    /* Return error code if realloc() failed */
    return (buckets == NULL) ? -1 : 0;
}

static c_dict_node_t **c_dict_getref(c_dict_base_t *dict, const char *key)
{
    unsigned hash = c_dict_hash(key);
    c_dict_node_t **next;
    if (dict->nbuckets > 0)
    {
        next = &dict->buckets[c_dict_bucketidx(dict, hash)];
        while (*next)
        {
            if ((*next)->hash == hash && !strcmp((char*) (*next + 1), key))
            {
                return next;
            }
            next = &(*next)->next;
        }
    }
    return NULL;
}

void c_dict_deinit_(c_dict_base_t *dict)
{
    c_dict_node_t *next, *node;
    int i;
    i = dict->nbuckets;
    while (i--)
    {
        node = dict->buckets[i];
        while (node)
        {
            next = node->next;
            free(node);
            node = next;
        }
    }
    free(dict->buckets);
}

void *c_dict_get_(c_dict_base_t *dict, const char *key)
{
    c_dict_node_t **next = c_dict_getref(dict, key);
    return next ? (*next)->value : NULL;
}

int c_dict_set_(c_dict_base_t *dict, const char *key, void *value, int vsize)
{
    int n, err;
    c_dict_node_t **next, *node;
    /* Find & replace existing node */
    next = c_dict_getref(dict, key);
    if (next)
    {
        memcpy((*next)->value, value, vsize);
        return 0;
    }
    /* Add new node */
    node = c_dict_newnode(key, value, vsize);
    if (!node)
    {
        return -1;
    }
    if (dict->nnodes >= dict->nbuckets)
    {
        n = (dict->nbuckets > 0) ? (dict->nbuckets << 1) : 1;
        err = c_dict_resize(dict, n);
        if (err)
        {
            free(node);
            return -1;
        }
    }
    c_dict_addnode(dict, node);
    dict->nnodes++;
    return 0;
}

void c_dict_remove_(c_dict_base_t *dict, const char *key)
{
    c_dict_node_t *node;
    c_dict_node_t **next = c_dict_getref(dict, key);
    if (next)
    {
        node = *next;
        *next = (*next)->next;
        free(node);
        dict->nnodes--;
    }
}

c_dict_iter_t c_dict_iter_(void)
{
    c_dict_iter_t iter;
    iter.bucketidx = -1;
    iter.node = NULL;
    return iter;
}

const char *c_dict_next_(c_dict_base_t *dict, c_dict_iter_t *iter)
{
    if (iter->node)
    {
        iter->node = iter->node->next;
        if (iter->node)
        {
            return (char*) (iter->node + 1);
        }
    }
    do
    {
        if (++iter->bucketidx >= dict->nbuckets)
        {
            return NULL;
        }
        iter->node = dict->buckets[iter->bucketidx];
    } while (!iter->node);
    return (char*) (iter->node + 1);
}
