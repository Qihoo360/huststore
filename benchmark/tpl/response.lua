function response(status, headers, body)
    if status ~= 200 then
        fails = fails + 1
        --print(string.format("uri: %s, code: %d", uri, status))
    end
end
