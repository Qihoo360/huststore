#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct
{
	ngx_str_t user;
	ngx_str_t passwd;
} htpasswd_t;

typedef struct htpasswd_node
{
	htpasswd_t item;
	struct htpasswd_node * next;
} htpasswd_node_t;

typedef struct
{
    ngx_http_complex_value_t   user_file;
    htpasswd_node_t * htpasswd_list;
} ngx_http_basic_auth_loc_conf_t;

static const ngx_str_t REALM = ngx_string("Authentication");

static ngx_int_t ngx_http_basic_auth_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_basic_auth_set_realm(ngx_http_request_t *r);
static void *ngx_http_basic_auth_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_basic_auth_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_basic_auth_init(ngx_conf_t *cf);
static char *ngx_http_basic_auth_user_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t  ngx_http_auth_basic_fast_commands[] =
{
    {
        ngx_string("http_basic_auth_file"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_basic_auth_user_file,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_basic_auth_loc_conf_t, user_file),
        NULL
    },
    ngx_null_command
};


static ngx_http_module_t  ngx_http_basic_auth_module_ctx =
{
    NULL,                                  /* preconfiguration */
    ngx_http_basic_auth_init,              /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_basic_auth_create_loc_conf,   /* create location configuration */
    ngx_http_basic_auth_merge_loc_conf     /* merge location configuration */
};


ngx_module_t  ngx_http_basic_auth_module =
{
    NGX_MODULE_V1,
    &ngx_http_basic_auth_module_ctx,       /* module context */
    ngx_http_auth_basic_fast_commands,     /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t __match(ngx_http_request_t * r, const htpasswd_t * item)
{
	if (!r || !item)
	{
		return NGX_ERROR;
	}
	if (item->user.len != r->headers_in.user.len)
	{
		return NGX_ERROR;
	}

	if (item->passwd.len != r->headers_in.passwd.len)
	{
		return NGX_ERROR;
	}

	if (0 != ngx_memcmp(r->headers_in.user.data, item->user.data, item->user.len))
	{
		return NGX_ERROR;
	}

	if (0 != ngx_memcmp(r->headers_in.passwd.data, item->passwd.data, item->passwd.len))
	{
		return NGX_ERROR;
	}

	return NGX_OK;
}

static ngx_int_t ngx_http_basic_auth_handler(ngx_http_request_t *r)
{
	ngx_http_basic_auth_loc_conf_t  * alcf = ngx_http_get_module_loc_conf(r, ngx_http_basic_auth_module);
	if (!alcf || !r)
	{
		return NGX_ERROR;
	}

	if (!alcf->htpasswd_list)
	{
		return NGX_DECLINED;
	}

	ngx_int_t rc = ngx_http_auth_basic_user(r);
	if (rc == NGX_DECLINED)
	{
		ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "no user/password was provided for basic authentication");
		return ngx_http_basic_auth_set_realm(r);
	}
	if (rc == NGX_ERROR)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	htpasswd_node_t * it = alcf->htpasswd_list;
	while (it)
	{
		if (NGX_OK == __match(r, &it->item))
		{
			return NGX_OK;
		}
		it = it->next;
	}

	return ngx_http_basic_auth_set_realm(r);
}

static ngx_int_t ngx_http_basic_auth_set_realm(ngx_http_request_t *r)
{
    size_t   len;
    u_char  *basic, *p;

    r->headers_out.www_authenticate = ngx_list_push(&r->headers_out.headers);
    if (r->headers_out.www_authenticate == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    len = sizeof("Basic realm=\"\"") - 1 + REALM.len;

    basic = ngx_pnalloc(r->pool, len);
    if (basic == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    p = ngx_cpymem(basic, "Basic realm=\"", sizeof("Basic realm=\"") - 1);
    p = ngx_cpymem(p, REALM.data, REALM.len);
    *p = '"';

    r->headers_out.www_authenticate->hash = 1;
    ngx_str_set(&r->headers_out.www_authenticate->key, "WWW-Authenticate");
    r->headers_out.www_authenticate->value.data = basic;
    r->headers_out.www_authenticate->value.len = len;

    return NGX_HTTP_UNAUTHORIZED;
}

static void * ngx_http_basic_auth_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_basic_auth_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_basic_auth_loc_conf_t));
    if (conf == NULL)
    {
        return NULL;
    }

    return conf;
}


static char * ngx_http_basic_auth_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_basic_auth_loc_conf_t  *prev = parent;
    ngx_http_basic_auth_loc_conf_t  *conf = child;

    if (conf->user_file.value.data == NULL)
    {
        conf->user_file = prev->user_file;
    }

    return NGX_CONF_OK;
}


static ngx_int_t ngx_http_basic_auth_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL)
    {
        return NGX_ERROR;
    }

    *h = ngx_http_basic_auth_handler;

    return NGX_OK;
}

static ngx_int_t __parse_node(const char * buf, ngx_pool_t * pool, htpasswd_t * item)
{
	char * pos = strstr(buf, ":");
	if (!pos)
	{
		return NGX_ERROR;
	}
	size_t len = strlen(buf);

	item->user.len = (size_t)(pos - buf);
	if (len < item->user.len + 2)
	{
		return NGX_ERROR;
	}

	item->passwd.len = len - item->user.len - 2;
	if (item->user.len < 1 || item->passwd.len < 1)
	{
		return NGX_ERROR;
	}

	item->user.data = ngx_palloc(pool, item->user.len + 1);
	if (!item->user.data)
	{
		return NGX_ERROR;
	}

	item->passwd.data = ngx_palloc(pool, item->passwd.len + 1);
	if (!item->passwd.data)
	{
		return NGX_ERROR;
	}

	memcpy(item->user.data, buf, item->user.len);
	item->user.data[item->user.len] = '\0';

	memcpy(item->passwd.data, pos + 1, item->passwd.len);
	item->passwd.data[item->passwd.len] = '\0';

	return NGX_OK;
}

static htpasswd_node_t * __parse(FILE * fp, ngx_pool_t * pool)
{
	enum { BUF_SIZE = 2048 };
	if (!pool || !fp)
	{
		return NULL;
	}

	htpasswd_node_t * list = NULL;
	htpasswd_node_t * it = NULL;
	char buf[BUF_SIZE];
	while (!feof(fp) && fgets(buf, BUF_SIZE, fp))
	{
		if ('#' == buf[0] || CR == buf[0] || LF == buf[0])
		{
			continue;
		}
		htpasswd_node_t * node = ngx_palloc(pool, sizeof(htpasswd_node_t));
		if (!node)
		{
			return NULL;
		}
		ngx_int_t rc = __parse_node(buf, pool, &node->item);
		if (NGX_ERROR == rc)
		{
			return NULL;
		}

		if (!list)
		{
			list = node;
			it = node;
			it->next = NULL;
		}
		else
		{
			it->next = node;
			it = it->next;
		}
	}

	if (it != list)
	{
		it = NULL;
	}

	return list;
}

static ngx_int_t __initialize(ngx_pool_t * pool, ngx_http_basic_auth_loc_conf_t * alcf)
{
	if (!pool || !alcf)
	{
		return NGX_ERROR;
	}
	alcf->htpasswd_list = NULL;

	if (!alcf->user_file.value.data)
	{
		return NGX_OK;
	}

	FILE * fp = fopen((const char *)alcf->user_file.value.data, "r");
	if (!fp)
	{
		return NGX_ERROR;
	}

	alcf->htpasswd_list = __parse(fp, pool);
	fclose(fp);

	if (!alcf->htpasswd_list)
	{
		return NGX_ERROR;
	}
	return NGX_OK;
}

static char * ngx_http_basic_auth_user_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_basic_auth_loc_conf_t *alcf = conf;

    ngx_str_t                         *value;
    ngx_http_compile_complex_value_t   ccv;

    if (alcf->user_file.value.data)
    {
        return "is duplicate";
    }

    value = cf->args->elts;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &alcf->user_file;
    ccv.zero = 1;
    ccv.conf_prefix = 1;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    if (NGX_ERROR == __initialize(cf->pool, alcf))
    {
    	return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
