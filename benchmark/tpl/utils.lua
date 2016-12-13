local tid = 1
local threads = {}
local addrs   = {}

wrk.method = "$var_method"
wrk.headers["Content-Type"] = "text/plain"
wrk.headers["Authorization"] = "Basic $var_auth"

function split(s, delimiter)
    result = {}
    for match in (s..delimiter):gmatch("(.-)"..delimiter) do
        table.insert(result, match)
    end
    return result
end

function file_exists(file)
    local f = io.open(file, "rb")
    if f then f:close() end
    return f ~= nil
end

function get_hosts(path)
    hosts = {}
    if not file_exists(path) then
        return hosts
    end
    for line in io.lines(path) do
        if not (line == "") then
            host = split(split(line, "\r")[1], "\n")[1]
            table.insert(hosts, host)
        end
    end
    return hosts
end

function get_key(id, requests)
    return string.format("benchmark_key_%d_%d", id, requests)
end

function get_hash_tb(id)
    return string.format("benchmark_hash_%d", id)
end

function get_sset_tb(id)
    return string.format("benchmark_sset_%d", id)
end

function get_zset_tb(id)
    return string.format("benchmark_zset_%d", id)
end

function get_queue(id)
    return string.format("benchmark_queue_%d", id)
end
