#include <dlfcn.h>
#include <pthread.h>
#include <ngx_http_fetch.h>
#include "hustdb_ha_table_def.h"
#include "hustdb_ha_handler.h"

static ngx_http_hustdb_ha_main_conf_t * g_mcf = NULL;

static ngx_int_t ngx_http_hustdb_ha_handler(ngx_http_request_t *r);
static char *ngx_http_hustdb_ha(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hustdb_ha_init_module(ngx_cycle_t * cycle);
static ngx_int_t ngx_http_hustdb_ha_init_process(ngx_cycle_t * cycle);
static void ngx_http_hustdb_ha_exit_process(ngx_cycle_t * cycle);
static void ngx_http_hustdb_ha_exit_master(ngx_cycle_t * cycle);
static char * ngx_http_debug_sync(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_zlog_mdc(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdbtable_file(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdb_ha_shm_name(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdb_ha_shm_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_public_pem(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_identifier_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_identifier_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_req_pool_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_keepalive_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_connection_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_connect_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_send_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_read_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_buffer_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_port(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_status_uri(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_user(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_passwd(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static void * ngx_http_hustdb_ha_create_main_conf(ngx_conf_t *cf);
char * ngx_http_hustdb_ha_init_main_conf(ngx_conf_t * cf, void * conf);

static ngx_http_request_item_t hustdb_ha_handler_dict[] =
{
    {
        ngx_string("/get"),
        ngx_string("/hustdb/get"),
        hustdb_ha_get_handler
    },
    {
        ngx_string("/get2"),
        ngx_string("/hustdb/get"),
        hustdb_ha_get2_handler
    },
    {
        ngx_string("/exist"),
        ngx_string("/hustdb/exist"),
        hustdb_ha_exist_handler
    },
    {
        ngx_string("/put"),
        ngx_string("/hustdb/put"),
        hustdb_ha_put_handler
    },
    {
        ngx_string("/del"),
        ngx_string("/hustdb/del"),
        hustdb_ha_del_handler
    },
    {
        ngx_string("/keys"),
        ngx_string("/hustdb/keys"),
        hustdb_ha_keys_handler
    },
    {
        ngx_string("/hset"),
        ngx_string("/hustdb/hset"),
        hustdb_ha_hset_handler
    },
    {
        ngx_string("/hget"),
        ngx_string("/hustdb/hget"),
        hustdb_ha_hget_handler
    },
    {
        ngx_string("/hget2"),
        ngx_string("/hustdb/hget"),
        hustdb_ha_hget2_handler
    },
    {
        ngx_string("/hdel"),
        ngx_string("/hustdb/hdel"),
        hustdb_ha_hdel_handler
    },
    {
        ngx_string("/hexist"),
        ngx_string("/hustdb/hexist"),
        hustdb_ha_hexist_handler
    },
    {
        ngx_string("/hkeys"),
        ngx_string("/hustdb/hkeys"),
        hustdb_ha_hkeys_handler
    },
    {
        ngx_string("/sadd"),
        ngx_string("/hustdb/sadd"),
        hustdb_ha_sadd_handler
    },
    {
        ngx_string("/srem"),
        ngx_string("/hustdb/srem"),
        hustdb_ha_srem_handler
    },
    {
        ngx_string("/sismember"),
        ngx_string("/hustdb/sismember"),
        hustdb_ha_sismember_handler
    },
    {
        ngx_string("/smembers"),
        ngx_string("/hustdb/smembers"),
        hustdb_ha_smembers_handler
    },
    {
        ngx_string("/file_count"),
        ngx_string("/hustdb/file_count"),
        hustdb_ha_file_count_handler
    },
    {
        ngx_string("/stat"),
        ngx_string("/hustdb/stat"),
        hustdb_ha_stat_handler
    },
    {
        ngx_string("/stat_all"),
        ngx_string("/hustdb/stat_all"),
        hustdb_ha_stat_all_handler
    },
    {
        ngx_string("/peer_count"),
        ngx_null_string,
        hustdb_ha_peer_count_handler
    },
    {
        ngx_string("/sync_status"),
        ngx_null_string,
        hustdb_ha_sync_status_handler
    },
    {
        ngx_string("/sync_alive"),
        ngx_null_string,
        hustdb_ha_sync_alive_handler
    },
    {
        ngx_string("/get_table"),
        ngx_null_string,
        hustdb_ha_get_table_handler
    },
    {
        ngx_string("/set_table"),
        ngx_null_string,
        hustdb_ha_set_table_handler
    },
    {
        ngx_string("/zismember"),
        ngx_string("/hustdb/zismember"),
        hustdb_ha_zismember_handler
    },
    {
        ngx_string("/zscore"),
        ngx_string("/hustdb/zscore"),
        hustdb_ha_zscore_handler
    },
    {
        ngx_string("/zscore2"),
        ngx_string("/hustdb/zscore"),
        hustdb_ha_zscore2_handler
    },
    {
        ngx_string("/zadd"),
        ngx_string("/hustdb/zadd"),
        hustdb_ha_zadd_handler
    },
    {
        ngx_string("/zrem"),
        ngx_string("/hustdb/zrem"),
        hustdb_ha_zrem_handler
    },
    {
        ngx_string("/zrangebyrank"),
        ngx_string("/hustdb/zrangebyrank"),
        hustdb_ha_zrangebyrank_handler
    },
    {
        ngx_string("/zrangebyscore"),
        ngx_string("/hustdb/zrangebyscore"),
        hustdb_ha_zrangebyscore_handler
    }
};

static size_t hustdb_ha_handler_dict_len = sizeof(hustdb_ha_handler_dict) / sizeof(ngx_http_request_item_t);

static ngx_command_t ngx_http_hustdb_ha_commands[] =
{
    {
        ngx_string("hustdb_ha"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_hustdb_ha,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    APPEND_MCF_ITEM("debug_sync", ngx_http_debug_sync),
    APPEND_MCF_ITEM("zlog_mdc", ngx_http_zlog_mdc),
    APPEND_MCF_ITEM("hustdbtable_file", ngx_http_hustdbtable_file),
    APPEND_MCF_ITEM("hustdb_ha_shm_name", ngx_http_hustdb_ha_shm_name),
    APPEND_MCF_ITEM("hustdb_ha_shm_size", ngx_http_hustdb_ha_shm_size),
    APPEND_MCF_ITEM("public_pem", ngx_http_public_pem),
    APPEND_MCF_ITEM("identifier_cache_size", ngx_http_identifier_cache_size),
    APPEND_MCF_ITEM("identifier_timeout", ngx_http_identifier_timeout),
    APPEND_MCF_ITEM("fetch_req_pool_size", ngx_http_fetch_req_pool_size),
    APPEND_MCF_ITEM("keepalive_cache_size", ngx_http_keepalive_cache_size),
    APPEND_MCF_ITEM("connection_cache_size", ngx_http_connection_cache_size),
    APPEND_MCF_ITEM("fetch_connect_timeout", ngx_http_fetch_connect_timeout),
    APPEND_MCF_ITEM("fetch_send_timeout", ngx_http_fetch_send_timeout),
    APPEND_MCF_ITEM("fetch_read_timeout", ngx_http_fetch_read_timeout),
    APPEND_MCF_ITEM("fetch_timeout", ngx_http_fetch_timeout),
    APPEND_MCF_ITEM("fetch_buffer_size", ngx_http_fetch_buffer_size),
    APPEND_MCF_ITEM("sync_port", ngx_http_sync_port),
    APPEND_MCF_ITEM("sync_status_uri", ngx_http_sync_status_uri),
    APPEND_MCF_ITEM("sync_user", ngx_http_sync_user),
    APPEND_MCF_ITEM("sync_passwd", ngx_http_sync_passwd),
    ngx_null_command
};

static ngx_http_module_t ngx_http_hustdb_ha_module_ctx =
{
    NULL, // ngx_int_t (*preconfiguration)(ngx_conf_t *cf);
    NULL, // ngx_int_t (*postconfiguration)(ngx_conf_t *cf);
    ngx_http_hustdb_ha_create_main_conf,
    ngx_http_hustdb_ha_init_main_conf,
    NULL, // void * (*create_srv_conf)(ngx_conf_t *cf);
    NULL, // char * (*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);
    NULL, // void * (*create_loc_conf)(ngx_conf_t *cf);
    NULL // char * (*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
};

ngx_module_t ngx_http_hustdb_ha_module =
{
    NGX_MODULE_V1,
    &ngx_http_hustdb_ha_module_ctx,
    ngx_http_hustdb_ha_commands,
    NGX_HTTP_MODULE,
    NULL, // ngx_int_t (*init_master)(ngx_log_t *log);
    ngx_http_hustdb_ha_init_module,
    ngx_http_hustdb_ha_init_process,
    NULL, // ngx_int_t (*init_thread)(ngx_cycle_t *cycle);
    NULL, // void (*exit_thread)(ngx_cycle_t *cycle);
    ngx_http_hustdb_ha_exit_process,
    ngx_http_hustdb_ha_exit_master,
    NGX_MODULE_V1_PADDING
};

static char * ngx_http_debug_sync(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        mcf->debug_sync = false;
        return NGX_CONF_OK;
    }
    int val = ngx_http_get_flag_slot(cf);
    if (NGX_ERROR == val)
    {
        return "ngx_http_debug_sync error";
    }
    mcf->debug_sync = val;
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_zlog_mdc(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_zlog_mdc error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->zlog_mdc = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_hustdbtable_file(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_hustdbtable_file error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->hustdbtable_file = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_hustdb_ha_shm_name(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_hustdb_ha_shm_name error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->hustdb_ha_shm_name = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_hustdb_ha_shm_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_hustdb_ha_shm_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->hustdb_ha_shm_size = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->hustdb_ha_shm_size)
    {
        return "ngx_http_hustdb_ha_shm_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_public_pem(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_public_pem error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->public_pem = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_identifier_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_identifier_cache_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->identifier_cache_size = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->identifier_cache_size)
    {
        return "ngx_http_identifier_cache_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_identifier_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_identifier_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->identifier_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->identifier_timeout)
    {
        return "ngx_http_identifier_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_req_pool_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_req_pool_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_req_pool_size = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->fetch_req_pool_size)
    {
        return "ngx_http_fetch_req_pool_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_keepalive_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_keepalive_cache_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->keepalive_cache_size = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->keepalive_cache_size)
    {
        return "ngx_http_keepalive_cache_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_connection_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_connection_cache_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->connection_cache_size = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->connection_cache_size)
    {
        return "ngx_http_connection_cache_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_connect_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_connect_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_connect_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->fetch_connect_timeout)
    {
        return "ngx_http_fetch_connect_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_send_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_send_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_send_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->fetch_send_timeout)
    {
        return "ngx_http_fetch_send_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_read_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_read_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_read_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->fetch_read_timeout)
    {
        return "ngx_http_fetch_read_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->fetch_timeout)
    {
        return "ngx_http_fetch_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_buffer_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_fetch_buffer_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->fetch_buffer_size = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->fetch_buffer_size)
    {
        return "ngx_http_fetch_buffer_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_port(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_port error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->sync_port = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->sync_port)
    {
        return "ngx_http_sync_port error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_status_uri(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_status_uri error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->sync_status_uri = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_user(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_user error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->sync_user = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_passwd(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_passwd error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->sync_passwd = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char *ngx_http_hustdb_ha(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t * clcf = ngx_http_conf_get_module_loc_conf(
        cf, ngx_http_core_module);
    clcf->handler = ngx_http_hustdb_ha_handler;
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_hustdb_ha_handler(ngx_http_request_t *r)
{
    ngx_http_request_item_t * it = ngx_http_get_request_item(
        hustdb_ha_handler_dict, hustdb_ha_handler_dict_len, &r->uri);
    if (!it)
    {
        return NGX_ERROR;
    }
    return it->handler(&it->backend_uri, r);
}

static ngx_int_t ngx_http_hustdb_ha_init_module(ngx_cycle_t * cycle)
{
    // TODO: initialize in master process
    return NGX_OK;
}

static ngx_int_t ngx_http_hustdb_ha_init_process(ngx_cycle_t * cycle)
{
    // TODO: initialize in worker process
    return NGX_OK;
}

static void ngx_http_hustdb_ha_exit_process(ngx_cycle_t * cycle)
{
    // TODO: uninitialize in worker process
}

static void ngx_http_hustdb_ha_exit_master(ngx_cycle_t * cycle)
{
    // TODO: uninitialize in master process
}

static void * ngx_http_hustdb_ha_create_main_conf(ngx_conf_t *cf)
{
    return ngx_pcalloc(cf->pool, sizeof(ngx_http_hustdb_ha_main_conf_t));
}

static ngx_int_t ngx_http_addon_init_shm_ctx(ngx_slab_pool_t * shpool, void * sh)
{
    hustdb_ha_shctx_t * ctx = (hustdb_ha_shctx_t *) sh;
    ctx->table_locked = false;
    if (!hustdb_ha_init_identifier_cache(g_mcf))
    {
        return NGX_ERROR;
    }
    return NGX_OK;
}

static ngx_bool_t __init_fetch(ngx_conf_t * cf, ngx_http_hustdb_ha_main_conf_t * mcf)
{
    static ngx_str_t ARGS = ngx_string("backend_count=");
    mcf->sync_status_args = hustdb_ha_strcat(&ARGS, (int) ngx_http_get_backend_count(), cf->pool);

    static ngx_str_t PREFIX = ngx_string("127.0.0.1:");
    ngx_str_t uri = hustdb_ha_strcat(&PREFIX, mcf->sync_port, cf->pool);
    if (!uri.data)
    {
        return false;
    }

    ngx_http_upstream_rr_peers_t  * peers = hustdb_ha_init_upstream_rr_peers(&uri, cf);
    if (!peers)
    {
        return false;
    }
    mcf->sync_peer = peers->peer;
    ngx_http_fetch_essential_conf_t ecf = { mcf->fetch_req_pool_size, mcf->keepalive_cache_size, mcf->connection_cache_size, cf, peers };
    ngx_http_fetch_upstream_conf_t ucf = {
        mcf->fetch_connect_timeout,
        mcf->fetch_send_timeout,
        mcf->fetch_read_timeout,
        mcf->fetch_timeout,
        mcf->fetch_buffer_size,
        { 0, 0 },
        0
    };
    if (NGX_OK != ngx_http_fetch_init_conf(&ecf, &ucf, NULL))
    {
        return false;
    }
    return true;
}

char * ngx_http_hustdb_ha_init_main_conf(ngx_conf_t * cf, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = conf;
    if (!mcf)
    {
        return NGX_CONF_ERROR;
    }
    mcf->pool = cf->pool;
    mcf->log = cf->log;
    mcf->prefix = cf->cycle->prefix;
    // TODO: you can initialize mcf here
    g_mcf = mcf;

    mcf->zone = ngx_http_addon_init_shm(cf, &mcf->hustdb_ha_shm_name, mcf->hustdb_ha_shm_size,
        sizeof(hustdb_ha_shctx_t), ngx_http_addon_init_shm_ctx, &ngx_http_hustdb_ha_module);

    hustdb_ha_init_peer_count(cf->pool);
	if (!hustdb_ha_init_peer_dict())
	{
		return NGX_CONF_ERROR;
	}

	if (!hustdb_ha_init_peer_array(cf->pool))
	{
	    return NGX_CONF_ERROR;
	}

	if (!hustdb_ha_init_log_dirs(&mcf->prefix, mcf->pool))
	{
	    return NGX_CONF_ERROR;
	}

	if (!__init_fetch(cf, mcf))
	{
	    return NGX_CONF_ERROR;
	}

	mcf->public_pem_full_path = ngx_http_get_conf_path(cf->cycle, &mcf->public_pem);
    
	ngx_str_t table_path = ngx_http_get_conf_path(cf->cycle, &mcf->hustdbtable_file);
	if (!table_path.data)
	{
		return NGX_CONF_ERROR;
	}
	if (!hustdb_ha_init_table_str(&table_path, cf->pool))
	{
	    return NGX_CONF_ERROR;
	}
	HustDbHaTable table;
	if (!cjson_load_hustdbhatable_from_file((const char *)table_path.data, &table))
	{
		return NGX_CONF_ERROR;
	}
	if (!table.json_has_table)
	{
	    return NGX_CONF_ERROR;
	}
	if (!hustdb_ha_build_table(&table, cf->pool))
	{
		return NGX_CONF_ERROR;
	}
	cjson_dispose_hustdbhatable(&table);

	hustdb_ha_init_table_path(table_path, cf->pool);

    return NGX_CONF_OK;
}

void * ngx_http_get_addon_module_ctx(ngx_http_request_t * r)
{
    if (!r)
    {
        return NULL;
    }
    return ngx_http_get_module_ctx(r, ngx_http_hustdb_ha_module);
}

void ngx_http_set_addon_module_ctx(ngx_http_request_t * r, void * ctx)
{
    if (!r)
    {
        return;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_hustdb_ha_module);
}

void * hustdb_ha_get_module_main_conf(ngx_http_request_t * r)
{
    if (!r)
    {
        return NULL;
    }
    return ngx_http_get_module_main_conf(r, ngx_http_hustdb_ha_module);
}
