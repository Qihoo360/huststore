#include <dlfcn.h>
#include <pthread.h>
#include "hustdb_ha_table_def.h"
#include "hustdb_ha_handler.h"

static void * g_libsync = NULL;
static ngx_http_hustdb_ha_main_conf_t * g_mcf = NULL;

static ngx_int_t ngx_http_hustdb_ha_handler(ngx_http_request_t *r);
static char *ngx_http_hustdb_ha(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hustdb_ha_init_module(ngx_cycle_t * cycle);
static ngx_int_t ngx_http_hustdb_ha_init_process(ngx_cycle_t * cycle);
static void ngx_http_hustdb_ha_exit_process(ngx_cycle_t * cycle);
static void ngx_http_hustdb_ha_exit_master(ngx_cycle_t * cycle);
static char * ngx_http_debug_sync(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_disable_sync(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_threads(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_release_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_checkdb_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_sync_checklog_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_zlog_mdc(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdbtable_file(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdb_ha_shm_name(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_hustdb_ha_shm_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_public_pem(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_identifier_cache_size(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
static char * ngx_http_identifier_timeout(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);
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
    APPEND_MCF_ITEM("disable_sync", ngx_http_disable_sync),
    APPEND_MCF_ITEM("sync_threads", ngx_http_sync_threads),
    APPEND_MCF_ITEM("sync_release_interval", ngx_http_sync_release_interval),
    APPEND_MCF_ITEM("sync_checkdb_interval", ngx_http_sync_checkdb_interval),
    APPEND_MCF_ITEM("sync_checklog_interval", ngx_http_sync_checklog_interval),
    APPEND_MCF_ITEM("zlog_mdc", ngx_http_zlog_mdc),
    APPEND_MCF_ITEM("hustdbtable_file", ngx_http_hustdbtable_file),
    APPEND_MCF_ITEM("hustdb_ha_shm_name", ngx_http_hustdb_ha_shm_name),
    APPEND_MCF_ITEM("hustdb_ha_shm_size", ngx_http_hustdb_ha_shm_size),
    APPEND_MCF_ITEM("public_pem", ngx_http_public_pem),
    APPEND_MCF_ITEM("identifier_cache_size", ngx_http_identifier_cache_size),
    APPEND_MCF_ITEM("identifier_timeout", ngx_http_identifier_timeout),
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

static char * ngx_http_disable_sync(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        mcf->disable_sync = false;
        return NGX_CONF_OK;
    }
    int val = ngx_http_get_flag_slot(cf);
    if (NGX_ERROR == val)
    {
        return "ngx_http_disable_sync error";
    }
    mcf->disable_sync = val;
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_threads(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_threads error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->sync_threads = ngx_atoi(value[1].data, value[1].len);
    if (NGX_ERROR == mcf->sync_threads)
    {
        return "ngx_http_sync_threads error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_release_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_release_interval error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->sync_release_interval = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->sync_release_interval)
    {
        return "ngx_http_sync_release_interval error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_checkdb_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_checkdb_interval error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->sync_checkdb_interval = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->sync_checkdb_interval)
    {
        return "ngx_http_sync_checkdb_interval error";
    }
    // TODO: you can modify the value here
    return NGX_CONF_OK;
}

static char * ngx_http_sync_checklog_interval(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_hustdb_ha_module);
    if (!mcf || 2 != cf->args->nelts)
    {
        return "ngx_http_sync_checklog_interval error";
    }
    ngx_str_t * value = cf->args->elts;
    mcf->sync_checklog_interval = ngx_parse_time(&value[1], 0);
    if (NGX_ERROR == mcf->sync_checklog_interval)
    {
        return "ngx_http_sync_checklog_interval error";
    }
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

static ngx_str_t __get_prefix(ngx_str_t * prefix, ngx_pool_t * pool)
{
    ngx_str_t path = { 0, 0 };
    path.len = prefix->len;
    path.data  = ngx_pcalloc(pool, path.len + 1);
    memcpy(path.data, prefix->data, prefix->len);
    path.data[path.len] = '\0';
    return path;
}

int ngx_before_master_cycle(ngx_cycle_t * cycle)
{
    if (g_mcf->disable_sync)
    {
        return NGX_OK;
    }
    ngx_str_t prefix = __get_prefix(&cycle->prefix, cycle->pool);
    static ngx_str_t libsync = ngx_string("sbin/libsync.so");
    ngx_str_t dl_path = hustdb_ha_init_dir(&prefix, &libsync, cycle->pool);

    static ngx_str_t logs = ngx_string("logs/");
    ngx_str_t logs_path = hustdb_ha_init_dir(&prefix, &logs, cycle->pool);

    static ngx_str_t auth = ngx_string("htpasswd");
    ngx_str_t auth_path = ngx_http_get_conf_path(cycle, &auth);

    do
    {
        g_libsync = dlopen((const char *)dl_path.data, RTLD_LAZY);
        if (!g_libsync)
        {
            break;
        }

        typedef ngx_bool_t (*libsync_init_t)(
            const char * logs_path,
            const char * ngx_path,
            const char * auth_path,
            int pool_size,
            int release_interval,
            int checkdb_interval,
            int checklog_interval);
        libsync_init_t init = dlsym(g_libsync, "init");
        char * error = dlerror();
        if(error)
        {
            break;
        }

        if (!init((const char *)logs_path.data,
            (const char *)prefix.data,
            (const char *)auth_path.data,
            g_mcf->sync_threads,
            g_mcf->sync_release_interval,
            g_mcf->sync_checkdb_interval,
            g_mcf->sync_checklog_interval))
        {
            break;
        }
        return NGX_OK;
    } while (0);

    if (g_libsync)
    {
        dlclose(g_libsync);
    }
    return NGX_ERROR;
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
    if (g_mcf->disable_sync || !g_libsync)
    {
        return;
    }
    typedef ngx_bool_t (*libsync_stop_sync_t)();
    libsync_stop_sync_t stop_sync = dlsym(g_libsync, "stop_sync");
    char * error = dlerror();
    if(!error)
    {
        stop_sync();
    }
    dlclose(g_libsync);
}

char * hustdb_ha_get_status()
{
    if (!g_libsync)
    {
        return NULL;
    }
    typedef char * (*libsync_get_status_t)(int hosts_size);
    libsync_get_status_t get_status = dlsym(g_libsync, "get_status");
    char * error = dlerror();
    if(!error)
    {
        return get_status(ngx_http_get_backend_count());
    }
    return NULL;
}

void hustdb_ha_dispose_status(char * status)
{
    if (!g_libsync)
    {
        return;
    }
    typedef void (*libsync_dispose_status_t)(char * status);
    libsync_dispose_status_t dispose_status = dlsym(g_libsync, "dispose_status");
    char * error = dlerror();
    if(!error)
    {
        dispose_status(status);
    }
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
