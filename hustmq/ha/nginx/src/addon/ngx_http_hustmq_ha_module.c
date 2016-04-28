#include "hustmq_ha_utils.h"
#include "hustmq_ha_handler.h"
#include "hustmq_ha_data_def.h"
#include "hustmq_ha_worker_def.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_fetch_stat.h"
#include "hustmq_ha_peer_def.h"

static ngx_int_t ngx_http_hustmq_ha_handler(ngx_http_request_t *r);
static char *ngx_http_hustmq_ha(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hustmq_ha_init_module(ngx_cycle_t * cycle);
static ngx_int_t ngx_http_hustmq_ha_init_process(ngx_cycle_t * cycle);
static void ngx_http_hustmq_ha_exit_process(ngx_cycle_t * cycle);
static void ngx_http_hustmq_ha_exit_master(ngx_cycle_t * cycle);
static char * ngx_http_max_queue_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_long_polling_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_subscribe_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_publish_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_max_timer_count(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_status_cache(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_req_pool_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_keepalive_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_connection_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_autost_uri(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_username(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_password(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_connect_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_send_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_read_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_fetch_buffer_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_autost_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_do_post_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_do_get_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_max_do_task_body_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_do_post_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_do_get_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static void * ngx_http_hustmq_ha_create_main_conf(ngx_conf_t *cf);
char * ngx_http_hustmq_ha_init_main_conf(ngx_conf_t * cf, void * conf);

static ngx_uint_t g_max_queue_size = 0;

static ngx_http_request_item_t hustmq_ha_handler_dict[] =
{
    {
        ngx_string("/worker"),
        ngx_string("/hustmq/worker"),
        hustmq_ha_worker_handler
    },
    {
        ngx_string("/put"),
        ngx_string("/hustmq/put"),
        hustmq_ha_put_handler
    },
    {
        ngx_string("/get"),
        ngx_string("/hustmq/get"),
        hustmq_ha_get_handler
    },
    {
        ngx_string("/autost"),
        ngx_string("/hustmq/stat_all"),
        hustmq_ha_autost_handler
    },
    {
        ngx_string("/purge"),
        ngx_string("/hustmq/purge"),
        hustmq_ha_purge_handler
    },
    {
        ngx_string("/max"),
        ngx_string("/hustmq/max"),
        hustmq_ha_max_handler
    },
    {
        ngx_string("/lock"),
        ngx_string("/hustmq/lock"),
        hustmq_ha_lock_handler
    },
    {
        ngx_string("/sub"),
        ngx_string("/hustmq/sub"),
        hustmq_ha_subscribe_handler
    },
    {
        ngx_string("/pub"),
        ngx_string("/hustmq/pub"),
        hustmq_ha_publish_handler
    },
    {
        ngx_string("/stat_all"),
        ngx_null_string,
        hustmq_ha_stat_all_handler
    },
    {
        ngx_string("/stat"),
        ngx_null_string,
        hustmq_ha_stat_handler
    },
    {
        ngx_string("/evget"),
        ngx_null_string,
        hustmq_ha_evget_handler
    },
    {
        ngx_string("/evsub"),
        ngx_null_string,
        hustmq_ha_evsub_handler
    },
    {
        ngx_string("/do_get"),
        ngx_null_string,
        hustmq_ha_do_get_handler
    },
    {
        ngx_string("/do_post"),
        ngx_null_string,
        hustmq_ha_do_post_handler
    },
    {
        ngx_string("/do_get_status"),
        ngx_null_string,
        hustmq_ha_do_get_status_handler
    },
    {
        ngx_string("/do_post_status"),
        ngx_null_string,
        hustmq_ha_do_post_status_handler
    }
};

static size_t hustmq_ha_handler_dict_len = sizeof(hustmq_ha_handler_dict) / sizeof(ngx_http_request_item_t);

static ngx_command_t ngx_http_hustmq_ha_commands[] =
{
    {
        ngx_string("hustmq_ha"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_hustmq_ha,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    APPEND_MCF_ITEM("max_queue_size", ngx_http_max_queue_size),
    APPEND_MCF_ITEM("long_polling_timeout", ngx_http_long_polling_timeout),
    APPEND_MCF_ITEM("subscribe_timeout", ngx_http_subscribe_timeout),
    APPEND_MCF_ITEM("publish_timeout", ngx_http_publish_timeout),
    APPEND_MCF_ITEM("max_timer_count", ngx_http_max_timer_count),
    APPEND_MCF_ITEM("status_cache", ngx_http_status_cache),
    APPEND_MCF_ITEM("fetch_req_pool_size", ngx_http_fetch_req_pool_size),
    APPEND_MCF_ITEM("keepalive_cache_size", ngx_http_keepalive_cache_size),
    APPEND_MCF_ITEM("connection_cache_size", ngx_http_connection_cache_size),
    APPEND_MCF_ITEM("autost_uri", ngx_http_autost_uri),
    APPEND_MCF_ITEM("username", ngx_http_username),
    APPEND_MCF_ITEM("password", ngx_http_password),
    APPEND_MCF_ITEM("fetch_connect_timeout", ngx_http_fetch_connect_timeout),
    APPEND_MCF_ITEM("fetch_send_timeout", ngx_http_fetch_send_timeout),
    APPEND_MCF_ITEM("fetch_read_timeout", ngx_http_fetch_read_timeout),
    APPEND_MCF_ITEM("fetch_timeout", ngx_http_fetch_timeout),
    APPEND_MCF_ITEM("fetch_buffer_size", ngx_http_fetch_buffer_size),
    APPEND_MCF_ITEM("autost_interval", ngx_http_autost_interval),
    APPEND_MCF_ITEM("do_post_cache_size", ngx_http_do_post_cache_size),
    APPEND_MCF_ITEM("do_get_cache_size", ngx_http_do_get_cache_size),
    APPEND_MCF_ITEM("max_do_task_body_size", ngx_http_max_do_task_body_size),
    APPEND_MCF_ITEM("do_post_timeout", ngx_http_do_post_timeout),
    APPEND_MCF_ITEM("do_get_timeout", ngx_http_do_get_timeout),
    ngx_null_command
};

static ngx_http_module_t ngx_http_hustmq_ha_module_ctx =
{
    NULL, // ngx_int_t (*preconfiguration)(ngx_conf_t *cf);
    NULL, // ngx_int_t (*postconfiguration)(ngx_conf_t *cf);
    ngx_http_hustmq_ha_create_main_conf,
    ngx_http_hustmq_ha_init_main_conf,
    NULL, // void * (*create_srv_conf)(ngx_conf_t *cf);
    NULL, // char * (*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);
    NULL, // void * (*create_loc_conf)(ngx_conf_t *cf);
    NULL // char * (*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
};

ngx_module_t ngx_http_hustmq_ha_module =
{
    NGX_MODULE_V1,
    &ngx_http_hustmq_ha_module_ctx,
    ngx_http_hustmq_ha_commands,
    NGX_HTTP_MODULE,
    NULL, // ngx_int_t (*init_master)(ngx_log_t *log);
    ngx_http_hustmq_ha_init_module,
    ngx_http_hustmq_ha_init_process,
    NULL, // ngx_int_t (*init_thread)(ngx_cycle_t *cycle);
    NULL, // void (*exit_thread)(ngx_cycle_t *cycle);
    ngx_http_hustmq_ha_exit_process,
    ngx_http_hustmq_ha_exit_master,
    NGX_MODULE_V1_PADDING
};

static char * ngx_http_max_queue_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_max_queue_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->max_queue_size = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->max_queue_size)
    {
        return "ngx_http_max_queue_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_long_polling_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_long_polling_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->long_polling_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->long_polling_timeout)
    {
        return "ngx_http_long_polling_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_subscribe_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_subscribe_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->subscribe_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->subscribe_timeout)
    {
        return "ngx_http_subscribe_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_publish_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_publish_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->publish_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->publish_timeout)
    {
        return "ngx_http_publish_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_max_timer_count(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_max_timer_count error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->max_timer_count = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->max_timer_count)
    {
        return "ngx_http_max_timer_count error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_status_cache(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_status_cache error";
    }
    int val = ngx_http_get_flag_slot(cf);
    if (NGX_ERROR == val)
    {
        return "ngx_http_status_cache error";
    }
    mcf->status_cache = val;
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_req_pool_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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

static char * ngx_http_autost_uri(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_autost_uri error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->autost_uri = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_username(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_username error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->username = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_password(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_password error";
    }
    ngx_str_t * arr = cf->args->elts;
    mcf->password = ngx_http_make_str(&arr[1], cf->pool);
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_fetch_connect_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
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

static char * ngx_http_autost_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_autost_interval error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->autost_interval = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->autost_interval)
    {
        return "ngx_http_autost_interval error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_do_post_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_do_post_cache_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->do_post_cache_size = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->do_post_cache_size)
    {
        return "ngx_http_do_post_cache_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_do_get_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_do_get_cache_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->do_get_cache_size = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->do_get_cache_size)
    {
        return "ngx_http_do_get_cache_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_max_do_task_body_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_max_do_task_body_size error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->max_do_task_body_size = ngx_parse_size(&value[1]);
    if (NGX_ERROR == mcf->max_do_task_body_size)
    {
        return "ngx_http_max_do_task_body_size error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_do_post_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_do_post_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->do_post_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->do_post_timeout)
    {
        return "ngx_http_do_post_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_do_get_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustmq_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_do_get_timeout error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->do_get_timeout = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->do_get_timeout)
    {
        return "ngx_http_do_get_timeout error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char *ngx_http_hustmq_ha(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t * clcf = ngx_http_conf_get_module_loc_conf(
        cf, ngx_http_core_module);
    clcf->handler = ngx_http_hustmq_ha_handler;
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_hustmq_ha_handler(ngx_http_request_t *r)
{
    ngx_http_request_item_t * it = ngx_http_get_request_item(
        hustmq_ha_handler_dict, hustmq_ha_handler_dict_len, &r->uri);
    if (!it)
    {
        return NGX_ERROR;
    }
    return it->handler(&it->backend_uri, r);
}

static ngx_int_t ngx_http_hustmq_ha_init_module(ngx_cycle_t * cycle)
{
    // TODO: initialize in master process
    return NGX_OK;
}

static ngx_int_t ngx_http_hustmq_ha_init_process(ngx_cycle_t * cycle)
{
    // TODO: initialize in worker process
    hustmq_ha_invoke_autost(cycle->log);
    return NGX_OK;
}

static void ngx_http_hustmq_ha_exit_process(ngx_cycle_t * cycle)
{
    // TODO: uninitialize in worker process
}

static void ngx_http_hustmq_ha_exit_master(ngx_cycle_t * cycle)
{
    // TODO: uninitialize in master process
}

static void * ngx_http_hustmq_ha_create_main_conf(ngx_conf_t *cf)
{
    return ngx_pcalloc(cf->pool, sizeof(ngx_http_hustmq_ha_main_conf_t));
}

char * ngx_http_hustmq_ha_init_main_conf(ngx_conf_t * cf, void * conf)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = conf;
    if (!mcf)
    {
        return NGX_CONF_ERROR;
    }
    mcf->pool = cf->pool;
    mcf->log = cf->log;
    mcf->prefix = cf->cycle->prefix;
    // TODO: you can initialize mcf here
    g_max_queue_size = mcf->max_queue_size;
	ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
	if (!hustmq_ha_init_peer_dict(peers))
    {
        return NGX_CONF_ERROR;
    }
	if (!hustmq_ha_init_peer_list(cf->pool, peers))
	{
	    return NGX_CONF_ERROR;
	}

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
	    return NGX_CONF_ERROR;
	}

	if (NGX_OK != hustmq_ha_init_stat_buffer(mcf))
	{
	    return NGX_CONF_ERROR;
	}

	if (NGX_OK != hustmq_ha_init_do_get(mcf))
	{
	    return NGX_CONF_ERROR;
	}

	if (NGX_OK != hustmq_ha_init_do_post(mcf))
    {
        return NGX_CONF_ERROR;
    }

	hustmq_ha_init_worker_buffer(cf->pool);
	hustmq_ha_init_evget_handler(cf->log);
	hustmq_ha_init_evsub_handler(cf->log);
	hustmq_ha_init_pub_ref_dict();
    return NGX_CONF_OK;
}

ngx_uint_t hustmq_ha_get_max_queue_size()
{
    return g_max_queue_size;
}

void * ngx_http_get_addon_module_ctx(ngx_http_request_t * r)
{
    if (!r)
    {
        return NULL;
    }
    return ngx_http_get_module_ctx(r, ngx_http_hustmq_ha_module);
}

void ngx_http_set_addon_module_ctx(ngx_http_request_t * r, void * ctx)
{
    if (!r)
    {
        return;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_hustmq_ha_module);
}

void * hustmq_ha_get_module_main_conf(ngx_http_request_t * r)
{
    if (!r)
    {
        return NULL;
    }
    return ngx_http_get_module_main_conf(r, ngx_http_hustmq_ha_module);
}
