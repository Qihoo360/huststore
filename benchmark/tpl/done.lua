function done(summary, latency, requests)
$var_head
    local total_requests = 0
    local total_fails = 0
    for index, thread in ipairs(threads) do
        local id = thread:get("id")
        local requests = thread:get("requests")
        total_requests = total_requests + requests
        local fails = thread:get("fails")
        total_fails = total_fails + fails
        local msg = "thread :%d, requests: %d, fails: %d"
        print(msg:format(id, requests, fails))
$var_mid
    end
    print(string.format("[summary] loop: %d, requests: %d, fails: %d", loop, total_requests, total_fails))
    set_loop("$var_loop", loop + 1)
$var_tail
end
