    --make_request
    uri = string.format("/$var_uri?tb=%s", get_sset_tb(id))
    local bufs = { [1] = body, [2] = get_key(id, requests) }
    wrk.body = table.concat(bufs, "")