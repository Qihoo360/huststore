#include "rdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {

	if(init(1024) != 0){
		return -1;
	}

    char resp[2048];
    size_t resp_len;

    client *c = createClient();

    ///set
    char *set_cmd[] = {"set", "name", "redis"};
    processInput(c, 3, set_cmd, &resp_len, resp);

        ///get
    char *get_cmd[] = {"get", "name"};
    processInput(c, 2, get_cmd, &resp_len, resp);
    //resp[resp_len] = '\0';
    printf("get value: %s %lu\n", resp, resp_len);

    ///exists
    char *exists_cmd[] = {"exists", "name"};
    processInput(c, 2, exists_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("exist value: %s %lu\n", resp, resp_len);

    ///del
    char *del_cmd[] = {"del", "name"};
    processInput(c, 2, del_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("del value: %s %lu\n", resp, resp_len);

    ///get deleted key
    char *get_del_cmd[] = {"get", "name"};
    processInput(c, 2, get_del_cmd, &resp_len, resp);
    printf("get_del value: %lu\n", resp_len);


    ///append
    char *append_cmd[] = {"append", "name", " rdb"};
    processInput(c, 3, append_cmd, &resp_len, resp);

    ///setex
    char *setex_cmd[] = {"setex", "cache_user_id", "60", "10086"};
    processInput(c, 4, setex_cmd, &resp_len, resp);

    sleep(1);

    ///ttl
    char *ttl_cmd[] = {"ttl", "cache_user_id"};
    processInput(c, 2, ttl_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("ttl value: %s %lu\n", resp, resp_len);

    ///hset
    char *hset_cmd[] = {"hset", "website", "google", "www.google.com"};
    processInput(c, 4, hset_cmd, &resp_len, resp);

    char *hset_cmd2[] = {"hset", "website", "facebook", "www.facebook.com"};
    processInput(c, 4, hset_cmd2, &resp_len, resp);

    ///hget
    char *hget_cmd[] = {"hget", "website", "google"};
    processInput(c, 3, hget_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("hget value: %s %lu\n", resp, resp_len);

    char *hget_cmd2[] = {"hget", "website", "facebook"};
    processInput(c, 3, hget_cmd2, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("hget value2: %s %lu\n", resp, resp_len);

    ///hdel
    char *hdel_cmd[] = {"hdel", "website", "facebook"};
    processInput(c, 3, hdel_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("hdel value: %s %lu\n", resp, resp_len);

    ///sadd
    char *sadd_cmd[] = {"sadd", "bbs", "discuz.net"};
    processInput(c, 3, sadd_cmd, &resp_len, resp);

    ///srandmember
    char *srandmember_cmd[] = {"srandmember", "bbs"};
    processInput(c, 2, srandmember_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("srandmember value: %s %lu\n", resp, resp_len);

    ///sismember
    char *sismember_cmd[] = {"sismember", "bbs", "discuz"};
    processInput(c, 3, sismember_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("sismember value: %s %lu\n", resp, resp_len);

    ///srem
    char *srem_cmd[] = {"srem", "bbs", "discuz.net"};
    processInput(c, 3, srem_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("srem value: %s %lu\n", resp, resp_len);

    ///zadd
    char *zadd_cmd[] = {"zadd", "page_rank", "10", "google.com"};
    processInput(c, 4, zadd_cmd, &resp_len, resp);

    ///info
    char *info_cmd[] = {"info", "all"};
    processInput(c, 2, info_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("info value: \n%s\n %lu\n", resp, resp_len);

    ///zscore
    char *zscore_cmd[] = {"zscore", "page_rank", "google.com"};
    processInput(c, 3, zscore_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zscore value: %s %lu\n", resp, resp_len);

    ///zrem
    char *zrem_cmd[] = {"zrem", "page_rank", "google.com"};
    processInput(c, 3, zrem_cmd, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zrem value: %s %lu\n", resp, resp_len);

    freeClient(c);

    return 0;
}