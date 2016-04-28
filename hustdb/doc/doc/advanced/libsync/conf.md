配置
--

`libsync`的配置依赖于 `HA` 的 `nginx` 配置。关于 `nginx` 的配置，请参看 [这里](../ha/nginx.md)

其中与 `libsync` 有关的配置项如下：

`disable_sync	on`	:	禁用`libsync`

`sync_threads	4`	:	使用4个线程进行数据同步

`sync_checkdb_interval	5s`	:	每隔5s检测后台db存活，只有后台db存活，才会开始同步

`sync_release_interval	5s`	: 	每隔5s检测日志文件是否同步完成，如果同步完成，会删除该日志文件。

`sync_checklog_interval 60s` : 	与 `zlog` 生成单一日志文件的速度保持一致，用于确定该日志文件是否已被写完

[上一级](../libsync.md)

[根目录](../../index.md)