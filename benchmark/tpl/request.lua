function request()
    requests = requests + 1
    if requests == max_requests - 1 then
        wrk.thread:stop()
    end
$var_make_request
    return wrk.format(nil, uri)
end
