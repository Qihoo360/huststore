    --make_request
    uri = string.format("/$var_uri?queue=%s", get_queue(id))
    local bufs = { [1] = body, [2] = get_key(id, requests) }
    wrk.body = table.concat(bufs, "")