    --make_request
    local key = get_key(loop, id, requests)
    uri = string.format("/$var_uri?key=%s", key)
    local bufs = { [1] = body, [2] = key }
    wrk.body = table.concat(bufs, "")