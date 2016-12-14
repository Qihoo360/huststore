    --make_request
    uri = string.format("/$var_uri?tb=%s&score=%d", get_zset_tb(id), (requests % 100) + 1)
    local bufs = { [1] = body, [2] = get_key(loop, id, requests) }
    wrk.body = table.concat(bufs, "")