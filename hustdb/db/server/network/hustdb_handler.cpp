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

bool post_handler(int r, std::string * rsp, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    return __post_handler(r, rsp ? rsp->c_str() : 0, rsp ? rsp->size() : 0, request, ctx);
}

}

void hustdb_exist_handler(const hustdb_exist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    int r = ctx->db->hustdb_exist(args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_get_handler(const hustdb_get_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    int r = ctx->db->hustdb_get(args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_put_handler(const hustdb_put_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_put(
        args.key.data, args.key.len, args.val.data, args.val.len, ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_del_handler(const hustdb_del_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_del(args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_keys_handler(const hustdb_keys_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_keys(args.offset, args.size, args.file, args.start, args.end,
        args.async, args.noval, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_stat_handler(const hustdb_stat_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
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

void hustdb_hexist_handler(const hustdb_hexist_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    int r = ctx->db->hustdb_hexist(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_hget_handler(const hustdb_hget_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    int r = ctx->db->hustdb_hget(args.tb.data, args.tb.len, args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_hset_handler(const hustdb_hset_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_hset(args.tb.data, args.tb.len, args.key.data, args.key.len, args.val.data, args.val.len,
        ver, args.ttl, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_hdel_handler(const hustdb_hdel_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_hdel(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_hkeys_handler(const hustdb_hkeys_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_hkeys(args.tb.data, args.tb.len, args.offset, args.size,
        args.start, args.end, args.async, args.noval, count, total, rsp, conn, ctxt );
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_sismember_handler(const hustdb_sismember_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    int r = ctx->db->hustdb_sismember(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_sadd_handler(const hustdb_sadd_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_sadd(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_srem_handler(const hustdb_srem_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_srem(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_smembers_handler(const hustdb_smembers_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
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

void hustdb_export_handler(const hustdb_export_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    void * token = 0;
    int r = ctx->db->hustdb_export(args.tb.data, args.tb.len, args.offset, args.size, args.file,
        args.start, args.end, args.cover, args.noval, token);
    uint64_t tmp = reinterpret_cast<uint64_t> (token);
    std::string rsp = evhtp::to_string(tmp);
    hustdb_network::post_handler(r, &rsp, request, ctx);
}

void hustmq_put_handler(const hustmq_put_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_put(args.queue.data, args.queue.len, args.item.data, args.item.len, args.priori, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_get_handler(const hustmq_get_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
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

    if (hustdb_network::post_handler(r, rsp, request, ctx) && args.ack)
    {
        ctx->db->hustmq_ack_inner(ack, conn);
    }
}

void hustmq_ack_handler(const hustmq_ack_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_ack(args.queue.data, args.queue.len, args.token.data, args.token.len, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_timeout_handler(const hustmq_timeout_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_timeout(args.queue.data, args.queue.len, (uint8_t) args.minute);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_worker_handler(const hustmq_worker_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string workers;
    int r = ctx->db->hustmq_worker(args.queue.data, args.queue.len, workers);
    hustdb_network::post_handler(r, &workers, request, ctx);
}

void hustmq_stat_handler(const hustmq_stat_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string stat;
    int r = ctx->db->hustmq_stat(args.queue.data, args.queue.len, stat);
    hustdb_network::post_handler(r, &stat, request, ctx);
}

void hustmq_stat_all_handler(evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    std::string stats;
    ctx->db->hustmq_stat_all(stats);
    evhtp::send_reply(ctx->db->errno_int_status(0), stats.c_str(), stats.size(), request);
}

void hustmq_max_handler(const hustmq_max_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_max(args.queue.data, args.queue.len, args.num);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_lock_handler(const hustmq_lock_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustmq_lock(args.queue.data, args.queue.len, args.on);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_purge_handler(const hustmq_purge_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_purge(args.queue.data, args.queue.len, args.priori, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_pub_handler(const hustmq_pub_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    conn_ctxt_t conn;
    conn.worker_id = ctx->base.get_id(request);
    int r = ctx->db->hustmq_pub(args.queue.data, args.queue.len, args.item.data, args.item.len, args.idx, args.ttl, conn);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(r), request);
}

void hustmq_sub_handler(const hustmq_sub_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
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
    hustdb_network::post_handler(r, rsp, request, ctx);
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

void hustdb_task_status_handler(const hustdb_task_status_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    uint64_t token = evhtp::cast <uint64_t> (std::string(args.token.data, args.token.len));
    slow_task_type_t task_type = ctx->db->slow_task_status((void *)token);
    std::string rsp = evhtp::to_string(task_type);
    evhtp::send_reply(ctx->db->errno_int_status(0), rsp.c_str(), rsp.size(), request);
}

void hustdb_zismember_handler(const hustdb_zismember_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_EXIST;
    int r = ctx->db->hustdb_zismember(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, conn, ctxt);
    hustdb_network::post_exist_handler(ver, r, request, ctx);
}

void hustdb_zscore_handler(const hustdb_zscore_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ;
    int r = ctx->db->hustdb_zscore(args.tb.data, args.tb.len, args.key.data, args.key.len, rsp, rsp_len, ver, conn, ctxt);
    hustdb_network::post_read_handler(ver, r, rsp, rsp_len, request, ctx);
}

void hustdb_zadd_handler(const hustdb_zadd_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    bool is_version_error = false;
    int r = ctx->db->hustdb_zadd(args.tb.data, args.tb.len, args.key.data, args.key.len, args.score, args.opt, ver, args.is_dup, conn, is_version_error);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, is_version_error, request);
}

void hustdb_zrem_handler(const hustdb_zrem_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_WRITE;
    int r = ctx->db->hustdb_zrem(args.tb.data, args.tb.len, args.key.data, args.key.len, ver, args.is_dup, conn, ctxt);
    hustdb_network::send_write_reply(ctx->db->errno_int_status(r), ver, ctxt, request);
}

void hustdb_zrangebyrank_handler(const hustdb_zrangebyrank_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_zrange(args.tb.data, args.tb.len, 0, 0, args.offset, args.size, args.start, args.end,
        args.async, args.noval, false, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_zrangebyscore_handler(const hustdb_zrangebyscore_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    PRE_READ_KEYS;
    int r = ctx->db->hustdb_zrange(args.tb.data, args.tb.len, args.min, args.max, args.offset, args.size, args.start, args.end,
        false, args.noval, true, count, total, rsp, conn, ctxt);
    hustdb_network::post_keys_handler(r, count, total, rsp, request, ctx);
}

void hustdb_sweep_handler(const hustdb_sweep_ctx_t& args, evhtp_request_t * request, hustdb_network_ctx_t * ctx)
{
    int r = ctx->db->hustdb_sweep(args.tb.data, args.tb.len);
    evhtp::send_nobody_reply(ctx->db->errno_int_status(0), request);
}
