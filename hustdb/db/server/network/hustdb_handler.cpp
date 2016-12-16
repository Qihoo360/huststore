#include "hustdb_handler.h"

#define PRE_HANDLER \
    conn_ctxt_t conn; \
    conn.worker_id = ctx->base.get_id(request); \
    item_ctxt_t * ctxt = NULL

#define PRE_EXIST \
    PRE_HANDLER; \
    uint32_t ver = 0

#define PRE_READ \
    PRE_EXIST; \
    std::string * rsp = NULL; \
    int rsp_len = 0

#define PRE_WRITE \
    PRE_HANDLER; \
    uint32_t ver = args.ver

#define PRE_READ_KEYS \
    PRE_HANDLER; \
    uint32_t count = 0; \
    std::string * rsp = NULL; \
    uint32_t total = 0

namespace hustdb_network {

void post_exist_handler(
    uint32_t ver,
    int r,
    evhtp_request_t * request,
    hustdb_network_ctx_t * ctx)
{
    hustdb_network::add_version(ver, request);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void post_read_handler(
    uint32_t ver,
    int r,
    std::string * rsp,
    int rsp_len,
    evhtp_request_t * request,
    hustdb_network_ctx_t * ctx)
{
    hustdb_network::add_version(ver, request);
    if (!r && rsp && rsp_len > 0)
    {
        evhtp::send_reply(ctx->db->errno_int_status(r), rsp->c_str(), rsp_len, request);
    }
    else
    {
        evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
    }
}

void add_total_keys(uint32_t keys, uint32_t total, evhtp_request_t * request)
{
    evhtp::add_numeric_kv("Keys", keys, request);
    evhtp::add_numeric_kv("TotalCount", total, request);
}

void post_keys_handler(
    int r,
    uint32_t keys,
    uint32_t total,
    std::string * rsp,
    evhtp_request_t * request,
    hustdb_network_ctx_t * ctx)
{
    if (!r && rsp && rsp->size() > 0)
    {
        add_total_keys(keys, total, request);
        evhtp::send_reply(ctx->db->errno_int_status(r), rsp->c_str(), rsp->size(), request);
    }
    else
    {
        evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
    }
}

bool __post_handler(int r, const char * data, size_t len, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    if (!r)
    {
        evhtp::send_reply(ctx->db->errno_int_status(r), data, len, request);
    }
    else
    {
        evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
    }
    return !r;
}

bool post_handler(int r, std::string * rsp, size_t size, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    return __post_handler(r, rsp ? rsp->c_str() : 0, size, request, ctx);
}

void unescape_key(bool ignore_post, evhtp_request_t * request, evhtp::c_str_t& key)
{
    if (ignore_post)
    {
        htp_method method = evhtp_request_get_method(request);
        if (htp_method_POST == method)
        {
            return;
        }
    }
    key.len = hustdb_unescape_str((char *)key.data, key.len);
}

}

void hustdb_exist_handler(hustdb_exist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_exist(args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_get_handler(hustdb_get_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_get(args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_put_handler(hustdb_put_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_put(
        args.key.data, args.key.len, args.val.data, args.val.len, ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_del_handler(hustdb_del_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_del(args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_keys_handler(hustdb_keys_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_keys(args.offset, args.size, args.file, args.start, args.end,
        args.async, args.noval, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_stat_handler(hustdb_stat_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int count = 0;
    int r = ctx->db->hustdb_stat(args.tb.data, args.tb.len, count);
    std::string tmp = evhtp::to_string(count);
    evhtp::send_reply(ctx->db->errno_int_status(r), tmp.c_str(), tmp.size(), request);
}

void hustdb_stat_all_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string stats;
    ctx->db->hustdb_stat_all(stats);
    evhtp::send_reply(ctx->db->errno_int_status(0), stats.c_str(), stats.size(), request);
}

void hustdb_hexist_handler(hustdb_hexist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_hexist(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_hget_handler(hustdb_hget_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_hget(args.tb.data, args.tb.len, args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_hset_handler(hustdb_hset_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_hset(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val.data, args.val.len,
        ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_hincrby_handler(hustdb_hincrby_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    ver = args.ver;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_hincrby(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val, args.host.data, args.host.len,
        rsp, rsp_len, ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::add_version(ver, request);
    hustdb_network::add_ver_err(ctxt, request);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}

void hustdb_hdel_handler(hustdb_hdel_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->hustdb_hdel(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_hkeys_handler(hustdb_hkeys_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_hkeys(args.tb.data, args.tb.len, args.offset, args.size,
        args.start, args.end, args.async, args.noval, count, total, rsp, conn, ctxt );
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_sismember_handler(hustdb_sismember_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    int r = ctx->db->hustdb_sismember(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_sadd_handler(hustdb_sadd_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_sadd(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_srem_handler(hustdb_srem_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_srem(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_smembers_handler(hustdb_smembers_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_smembers(args.tb.data, args.tb.len, args.offset, args.size,
        args.start, args.end, args.async, args.noval, count, total, rsp, conn, ctxt );
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_file_count_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int file_count = ctx->db->hustdb_file_count();
    std::string tmp = evhtp::to_string(file_count);
    evhtp::send_reply(ctx->db->errno_int_status(0), tmp.c_str(), tmp.size(), request);
}

void hustdb_export_handler(hustdb_export_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    void * token = 0;
    int r = ctx->db->hustdb_export(args.tb.data, args.tb.len, args.offset, args.size, args.file,
        args.start, args.end, args.cover, args.noval, token);
    uint64_t tmp = reinterpret_cast<uint64_t> (token);
    std::string rsp = evhtp::to_string(tmp);
    hustdb_network::post_handler(r, &rsp, rsp.size(), request, ctx);
}

void hustdb_binlog_handler(hustdb_binlog_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustdb_binlog(args.has_tb ? args.tb.data : NULL, args.has_tb ? args.tb.len : 0, args.key.data, args.key.len, args.host.data, args.host.len, args.method, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_put_handler(hustmq_put_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_put(args.queue.data, args.queue.len, args.item.data, args.item.len, args.priori, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_get_handler(hustmq_get_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    std::string ack;
    std::string unacked;
    std::string * rsp = NULL;
    int r = ctx->db->hustmq_get(args.queue.data, args.queue.len, args.worker.data, args.worker.len, args.ack, ack, unacked, rsp, conn);

    if (!args.ack)
    {
        evhtp::add_kv("Ack-Token", unacked.c_str(), request);
    }

    if (hustdb_network::post_handler(r, rsp, rsp ? rsp->size() : 0, request, ctx) && args.ack)
    {
        ctx->db->hustmq_ack_inner(ack, conn);
    }
}

void hustmq_ack_handler(hustmq_ack_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_ack(args.queue.data, args.queue.len, args.token.data, args.token.len, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_timeout_handler(hustmq_timeout_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_timeout(args.queue.data, args.queue.len, (uint8_t) args.minute);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_worker_handler(hustmq_worker_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string workers;
    int r = ctx->db->hustmq_worker(args.queue.data, args.queue.len, workers);
    hustdb_network::post_handler(r, &workers, workers.size(), request, ctx);
}

void hustmq_stat_handler(hustmq_stat_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string stat;
    int r = ctx->db->hustmq_stat(args.queue.data, args.queue.len, stat);
    hustdb_network::post_handler(r, &stat, stat.size(), request, ctx);
}

void hustmq_stat_all_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string stats;
    ctx->db->hustmq_stat_all(stats);
    evhtp::send_reply(ctx->db->errno_int_status(0), stats.c_str(), stats.size(), request);
}

void hustmq_max_handler(hustmq_max_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_max(args.queue.data, args.queue.len, args.num);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_lock_handler(hustmq_lock_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_lock(args.queue.data, args.queue.len, args.on);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_purge_handler(hustmq_purge_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_purge(args.queue.data, args.queue.len, args.priori, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_pub_handler(hustmq_pub_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_pub(args.queue.data, args.queue.len, args.item.data, args.item.len, args.idx, args.ttl, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_sub_handler(hustmq_sub_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    std::string * rsp = NULL;
    uint32_t sp = 0;
    uint32_t ep = 0;
    int r = ctx->db->hustmq_sub(args.queue.data, args.queue.len, args.idx, sp, ep, rsp, conn);
    if (0 != sp || 0 != ep)
    {
        char val[23] = {0};
        sprintf(val, "%u-%u", sp, ep);
        evhtp::add_kv("Index", val, request);
    }
    hustdb_network::post_handler(r, rsp, rsp ? rsp->size() : 0, request, ctx);
}

void hustdb_info_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string info;
    ctx->db->hustdb_info(info);
    evhtp::send_reply(ctx->db->errno_int_status(0), info.c_str(), info.size(), request);
}

void hustdb_task_info_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string info;
    ctx->db->slow_task_info(info);
    evhtp::send_reply(ctx->db->errno_int_status(0), info.c_str(), info.size(), request);
}

void hustdb_task_status_handler(hustdb_task_status_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    uint64_t token = evhtp::cast <uint64_t> (std::string(args.token.data, args.token.len));
    slow_task_type_t task_type = ctx->db->slow_task_status((void *)token);
    std::string rsp = evhtp::to_string(task_type);
    evhtp::send_reply(ctx->db->errno_int_status(0), rsp.c_str(), rsp.size(), request);
}

void hustdb_zismember_handler(hustdb_zismember_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    hustdb_network::unescape_key(true, request, args.key);
    int r = ctx->db->hustdb_zismember(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_zscore_handler(hustdb_zscore_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(true, request, args.key);
    int r = ctx->db->hustdb_zscore(args.tb.data, args.tb.len, args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_zadd_handler(hustdb_zadd_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    bool is_version_error = false;
    hustdb_network::unescape_key(true, request, args.key);
    int r = ctx->db->hustdb_zadd(args.tb.data, args.tb.len, args.key.data, args.key.len, args.score, args.opt, ver, args.ttl, args.is_dup, conn, is_version_error);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, is_version_error, request);
}

void hustdb_zrem_handler(hustdb_zrem_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    hustdb_network::unescape_key(true, request, args.key);
    int r = ctx->db->hustdb_zrem(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_zrangebyrank_handler(hustdb_zrangebyrank_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_zrange(args.tb.data, args.tb.len, 0, 0, args.offset, args.size, args.start, args.end,
        args.async, args.noval, false, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_zrangebyscore_handler(hustdb_zrangebyscore_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_zrange(args.tb.data, args.tb.len, args.min, args.max, args.offset, args.size, args.start, args.end,
        false, args.noval, true, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustcache_exist_handler(hustcache_exist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->exist_or_del(args.key.data, args.key.len, true, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_get_handler(hustcache_get_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->get_or_ttl(args.key.data, args.key.len, rsp, &rsp_len, true, conn);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}

void hustcache_ttl_handler(hustcache_ttl_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->get_or_ttl(args.key.data, args.key.len, rsp, &rsp_len, false, conn);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}

void hustcache_put_handler(hustcache_put_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->set_or_append(args.key.data, args.key.len, args.val.data, args.val.len, args.ttl.data, args.ttl.len, true, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_append_handler(hustcache_append_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->set_or_append(args.key.data, args.key.len, args.val.data, args.val.len, NULL, 0, false, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_del_handler(hustcache_del_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->exist_or_del(args.key.data, args.key.len, false, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_expire_handler(hustcache_expire_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->expire_or_persist(args.key.data, args.key.len, args.ttl.data, args.ttl.len, true, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_persist_handler(hustcache_persist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->expire_or_persist(args.key.data, args.key.len, NULL, 0, false, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_hexist_handler(hustcache_hexist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hexist_or_hdel(args.tb.data, args.tb.len, args.key.data, args.key.len, true, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_hget_handler(hustcache_hget_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hget(args.tb.data, args.tb.len, args.key.data, args.key.len, rsp, &rsp_len, conn);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}

void hustcache_hset_handler(hustcache_hset_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hset(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val.data, args.val.len, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_hdel_handler(hustcache_hdel_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_HANDLER;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hexist_or_hdel(args.tb.data, args.tb.len, args.key.data, args.key.len, false, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustcache_hincrby_handler(hustcache_hincrby_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hincrby_or_hincrbyfloat(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val.data, args.val.len, rsp, &rsp_len, true, conn);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}

void hustcache_hincrbyfloat_handler(hustcache_hincrbyfloat_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    hustdb_network::unescape_key(false, request, args.key);
    int r = ctx->db->get_rdb()->hincrby_or_hincrbyfloat(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val.data, args.val.len, rsp, &rsp_len, false, conn);
    hustdb_network::post_handler(r, rsp, rsp_len, request, ctx);
}
