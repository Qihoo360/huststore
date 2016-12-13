    -- tail
    body = load_body("data")
    local msg = "thread %d created, max_requests: %d"
    print(msg:format(id, max_requests))