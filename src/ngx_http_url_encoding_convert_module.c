/*
 * @file:    ngx_http_url_encoding_convert_module.c
 * @brief:   The url encoding converting module.
 * @author:  YoungJoo.Kim <vozlt@vozlt.com>
 * @version:
 * @date:
 *
 * Compile:
 *           shell> ./configure --add-module=/path/to/nginx-module-url
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <iconv.h>

#define NGX_URL_ENCODING_CONVERT_DEFAULT_FROM_ENCODE    "utf-8"
#define NGX_URL_ENCODING_CONVERT_DEFAULT_TO_ENCODE      "euc-kr"


typedef struct {
    size_t          alloc_size_x;
    size_t          alloc_size;
    ngx_uint_t      phase;
    ngx_flag_t      enable;
    ngx_str_t       from_encode;
    ngx_str_t       to_encode;
} ngx_http_url_encoding_convert_loc_conf_t;


static void *ngx_http_url_encoding_convert_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_url_encoding_convert_merge_loc_conf(ngx_conf_t *cf,
        void *parent, void *child);
static ngx_int_t ngx_http_url_encoding_convert_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_url_encoding_convert_iconv(ngx_http_request_t *r,
        const char *in_charset, const char *out_charset);


static ngx_conf_enum_t  ngx_http_url_encoding_convert_alloc_size_x[] = {
    { ngx_string("x4"), 4 }, { ngx_string("x5"), 5 },
    { ngx_string("x6"), 6 }, { ngx_string("x7"), 7 },
    { ngx_string("x8"), 8 }, { ngx_string("x9"), 9 },
    { ngx_string("x10"), 10 }, { ngx_string("x11"), 11 },
    { ngx_string("x12"), 12 }, { ngx_string("x13"), 13 },
    { ngx_string("x14"), 14 }, { ngx_string("x15"), 15 },
    { ngx_string("x16"), 16 }, { ngx_null_string, 0 }
};


static ngx_conf_enum_t  ngx_http_url_encoding_convert_phase[] = {
    { ngx_string("post_read"), NGX_HTTP_POST_READ_PHASE }, /* post_read: It works before rewrite */
    { ngx_string("preaccess"), NGX_HTTP_PREACCESS_PHASE }, /* preaccess: It works after rewrite */
    { ngx_null_string, 0 }
};


static ngx_command_t ngx_http_url_encoding_convert_commands[] = {

    /* on|off */
    { ngx_string("url_encoding_convert"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_url_encoding_convert_loc_conf_t, enable),
        NULL },

    /* handler's phase to working: NGX_HTTP_POST_READ_PHASE | NGX_HTTP_PREACCESS_PHASE */
    { ngx_string("url_encoding_convert_phase"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_enum_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_url_encoding_convert_loc_conf_t, phase),
        &ngx_http_url_encoding_convert_phase },

    /* x[4-16]: iconv_buffer_size = r->uri.len * alloc_size_x */
    { ngx_string("url_encoding_convert_alloc_size_x"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_enum_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_url_encoding_convert_loc_conf_t, alloc_size_x),
        &ngx_http_url_encoding_convert_alloc_size_x },

    /* iconv_buffer_size = alloc_size */
    { ngx_string("url_encoding_convert_alloc_size"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_url_encoding_convert_loc_conf_t, alloc_size),
        NULL },

    /* set client's encoding */
    { ngx_string("url_encoding_convert_from"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_url_encoding_convert_loc_conf_t, from_encode),
      NULL },

    /* set server's encoding */
    { ngx_string("url_encoding_convert_to"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_url_encoding_convert_loc_conf_t, to_encode),
      NULL },

    ngx_null_command
};


static ngx_http_module_t ngx_http_url_encoding_convert_module_ctx = {
    NULL,                                           /* preconfiguration */
    ngx_http_url_encoding_convert_init,             /* postconfiguration */

    NULL,                                           /* create main configuration */
    NULL,                                           /* init main configuration */

    NULL,                                           /* create server configuration */
    NULL,                                           /* merge server configuration */

    ngx_http_url_encoding_convert_create_loc_conf,  /* create location configuration */
    ngx_http_url_encoding_convert_merge_loc_conf,   /* merge location configuration */
};


ngx_module_t ngx_http_url_encoding_convert_module = {
    NGX_MODULE_V1,
    &ngx_http_url_encoding_convert_module_ctx,   /* module context */
    ngx_http_url_encoding_convert_commands,      /* module directives */
    NGX_HTTP_MODULE,                             /* module type */
    NULL,                                        /* init master */
    NULL,                                        /* init module */
    NULL,                                        /* init process */
    NULL,                                        /* init thread */
    NULL,                                        /* exit thread */
    NULL,                                        /* exit process */
    NULL,                                        /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_url_encoding_convert_handler(ngx_http_request_t *r)
{
    u_char                                      *last;
    size_t                                      root;
    ngx_int_t                                   rc;
    ngx_str_t                                   path;
    ngx_file_info_t                             of;
    ngx_http_url_encoding_convert_loc_conf_t    *ulcf;

    ulcf = ngx_http_get_module_loc_conf(r, ngx_http_url_encoding_convert_module);

    if (!ulcf->enable) {
         return NGX_DECLINED;
    }

    /* before rewrite */
    if (r->phase_handler == NGX_HTTP_POST_READ_PHASE && ulcf->phase == NGX_HTTP_PREACCESS_PHASE) {
         return NGX_DECLINED;
    }

    /* after rewrite */
    if (r->phase_handler > NGX_HTTP_POST_READ_PHASE && ulcf->phase == NGX_HTTP_POST_READ_PHASE) {
         return NGX_DECLINED;
    }

    last = ngx_http_map_uri_to_path(r, &path, &root, 0);
    if (last == NULL) {
        return NGX_DECLINED;
    }

    path.len = last - path.data;

    if (ngx_file_info(path.data, &of) == NGX_FILE_ERROR) {
        rc = ngx_http_url_encoding_convert_iconv(r, (const char *)ulcf->from_encode.data, (const char *)ulcf->to_encode.data);

        if (rc == NGX_OK) {
            return rc;
        }
    }

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_url_encoding_convert_iconv(ngx_http_request_t *r,
        const char *in_charset, const char *out_charset)
{
    size_t                                      rv, alloc_size;
    ngx_buf_t                                   *b;
    ngx_str_t                                   uri;
    ngx_http_url_encoding_convert_loc_conf_t    *ulcf;
    iconv_t                                     cd;

    cd = iconv_open((const char *)out_charset, (const char *)in_charset);

    if (cd == (iconv_t) -1) {
        return NGX_ERROR;
    }

    ulcf = ngx_http_get_module_loc_conf(r, ngx_http_url_encoding_convert_module);

    alloc_size = (ulcf->alloc_size_x * r->uri.len + 1);
    alloc_size = alloc_size > ulcf->alloc_size ? alloc_size : ulcf->alloc_size;

    b = ngx_create_temp_buf(r->pool, alloc_size);
    if (b == NULL) {
        iconv_close(cd);
        return NGX_ERROR;
    }

    uri = r->uri;

    rv = iconv(cd, (void *) &uri.data, &uri.len, (void *) &b->last, &ngx_pagesize);

    if (rv == (size_t) -1) {
        iconv_close(cd);
        return NGX_ERROR;
    }

    r->uri.data = b->pos;
    r->uri.len = b->last - b->pos;

    iconv_close(cd);
    return NGX_OK;
}


static void *
ngx_http_url_encoding_convert_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_url_encoding_convert_loc_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_url_encoding_convert_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->alloc_size_x = NGX_CONF_UNSET_SIZE;
    conf->alloc_size = NGX_CONF_UNSET_SIZE;
    conf->phase = NGX_CONF_UNSET_UINT;
    conf->enable = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_url_encoding_convert_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{   
    ngx_http_url_encoding_convert_loc_conf_t *prev = parent;
    ngx_http_url_encoding_convert_loc_conf_t *conf = child;

    ngx_conf_merge_size_value(conf->alloc_size_x, prev->alloc_size_x, 4);
    ngx_conf_merge_size_value(conf->alloc_size, prev->alloc_size, 0);
    ngx_conf_merge_uint_value(conf->phase, prev->phase, NGX_HTTP_PREACCESS_PHASE);
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_str_value(conf->from_encode, prev->from_encode,
        NGX_URL_ENCODING_CONVERT_DEFAULT_FROM_ENCODE);
    ngx_conf_merge_str_value(conf->to_encode, prev->to_encode,
        NGX_URL_ENCODING_CONVERT_DEFAULT_TO_ENCODE);

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_url_encoding_convert_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    /* before rewrite */
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_url_encoding_convert_handler;

    /* after rewrite */
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_url_encoding_convert_handler;

    return NGX_OK;
}
