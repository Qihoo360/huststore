libsync
--

数据同步模块。

`HA` 在写入后端db失败时，会将数据写入本地日志，`libsync` 负责将这些数据同步到后端db中。

项目目录： `hustdb/sync`

项目模块：

* `libsync` 模块 : hustdb/sync/libsync

`libsync` 模块依赖于 [libcurl](https://curl.haxx.se)

[上一级](index.md)

[根目录](../index.md)