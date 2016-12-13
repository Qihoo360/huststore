function get_requests(path, tid)
    if not file_exists(path) then
        return 1048576
    end
    for line in io.lines(path) do
        if not (line == "") then
            words = split(line, " ")
            if tid == tonumber(words[1]) then
                return tonumber(words[2])
            end
        end
    end
    return 1048576
end
