    --make_request
    local tb = get_hash_tb(id)
    local key = get_key(id, requests)
    uri = string.format("/$var_uri?tb=%s&key=%s", tb, key)
    local bufs = { [1] = body, [2] = key }
    wrk.body = table.concat(bufs, "")