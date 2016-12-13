function load_body(path)
    local file = io.open(path, "r")
    assert(file)
    local body = file:read("*a")
    file:close()
    return string.sub(body, 1, -2)
end
