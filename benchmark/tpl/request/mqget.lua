    --make_request
    uri = string.format("/$var_uri?queue=%s&worker=benchmark_worker_%d", get_queue(id), id)