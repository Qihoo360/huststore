#include "rdb_in.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, char **argv) {

	if(init(256) != 0){
		return -1;
	}

    char resp[2048];
    size_t resp_len;

    client *c = createClient(65535);

    ///set
    char *set_cmd[] = {"set", "name", "redis"};
    RdbCommand set_commands[3];
    for(int i = 0; i < 3; i++){
        set_commands[i].cmd = set_cmd[i];
        set_commands[i].len = strlen(set_cmd[i]);
    }
    processInput(c, 3, set_commands, &resp_len, resp);
    //processInput(c, 3, set_cmd, &resp_len, resp);

    ///get
    char *get_cmd[] = {"get", "name"};
    RdbCommand get_commands[2];
    for(int i = 0; i < 2; i++){
        get_commands[i].cmd = get_cmd[i];
        get_commands[i].len = strlen(get_cmd[i]);
    }
    processInput(c, 2, get_commands, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("get value: %s %lu\n", resp, resp_len);

    ///zadd
    char *zadd_cmd[] = {"zadd", "page_rank", "10", "google.com"};
    RdbCommand zadd_commands[4];
    for(int i = 0; i < 4; i++){
        zadd_commands[i].cmd = zadd_cmd[i];
        zadd_commands[i].len = strlen(zadd_cmd[i]);
    }
    processInput(c, 4, zadd_commands, &resp_len, resp);

    char *zadd_cmd_1[] = {"zadd", "page_rank", "11", "facebook.com"};
    RdbCommand zadd_commands_1[4];
    for(int i = 0; i < 4; i++){
        zadd_commands_1[i].cmd = zadd_cmd_1[i];
        zadd_commands_1[i].len = strlen(zadd_cmd_1[i]);
    }
    processInput(c, 4, zadd_commands_1, &resp_len, resp);
    /*
    char *zrange_cmd[] = {"zrange", "page_rank", "0", "1", "withscores"};
    RdbCommand zrange_commands[5];
    for(int i = 0; i < 5; i++){
        zrange_commands[i].cmd = zrange_cmd[i];
        zrange_commands[i].len = strlen(zrange_cmd[i]);
    }
    processInput(c, 5, zrange_commands, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zrange:%s\n", resp);
    */

    char *zcard_cmd[] = {"zcard", "page_rank"};
    RdbCommand zcard_commands[2];
    for(int i = 0; i < 2; i++){
        zcard_commands[i].cmd = zcard_cmd[i];
        zcard_commands[i].len = strlen(zcard_cmd[i]);
    }
    processInput(c, 2, zcard_commands, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zcard: %s\n", resp);

   
    char *zremrangebyrank_cmd[] = {"zincrby", "page_rank", "3", "google.com"};
    RdbCommand zremrangebyrank_commands[4];
    for(int i = 0; i < 4; i++){
        zremrangebyrank_commands[i].cmd = zremrangebyrank_cmd[i];
        zremrangebyrank_commands[i].len = strlen(zremrangebyrank_cmd[i]);
    }
    processInput(c, 4, zremrangebyrank_commands, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zincrby:%s\n", resp);

    char *zrange_by_score_cmd[] = {"zrangebyscore", "page_rank", "1", "14", "withscores"};
    RdbCommand zrange_by_score_commands[5];
    for(int i = 0; i < 5; i++){
        zrange_by_score_commands[i].cmd = zrange_by_score_cmd[i]; 
        zrange_by_score_commands[i].len = strlen(zrange_by_score_cmd[i]);
    }
    processInput(c, 5, zrange_by_score_commands, &resp_len, resp);
    resp[resp_len] = '\0';
    printf("zrangebyscore:%s\n", resp);
    /*

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
    */

    freeClient(c);

    return 0;
}