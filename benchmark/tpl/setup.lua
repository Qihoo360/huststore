function setup(thread)
    local append = function(host, port)
        for i, addr in ipairs(wrk.lookup(host, port)) do
            if wrk.connect(addr) then
                addrs[#addrs+1] = addr
            end
        end
    end
    if #addrs == 0 then
        local hosts = get_hosts("hosts")
        for i, host in ipairs(hosts) do
            local items = split(host, ":")
            append(items[1], tonumber(items[2]))
        end
    end
    if #addrs > 0 then
        local idx = tid % #addrs + 1
        thread.addr = addrs[idx]
        print(string.format("thread %d -> %s", tid, thread.addr))
    end
    thread:set("id", tid)
    table.insert(threads, thread)
    tid = tid + 1
end
