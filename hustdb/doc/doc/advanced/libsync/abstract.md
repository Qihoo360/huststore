概述
--

`libsync` 模块用于将本地日志同步到后端db中。由以下子模块组成。

### check_backend ###

功能：定时检测后端db存活。

如果db可用，则通过 `pipe` 通知 `read_log` 模块读取该db下的日志文件，开始同步。

### release_file ###

功能： 定时清理同步完成的文件

通过位图检测 `release_queue` 中的文件是否同步完成，如果同步完成则删除文件并释放资源。

### monitor ###

功能：监控日志文件目录。

如果产生日志文件，则将文件加入对应db的 `file_queue`。

### read_log ###

功能：读取日志文件。

从 `file_queue` 中取出日志文件，按行读取该日志文件，将数据加入线程池的 `data_queue`。

将读取完成的文件加入到 `release_queue` 中。

### sync_threadpool ###

功能：线程池，同步数据。

线程池中线程从 `data_queue` 中取出数据，进行base64解码，构造url，query等，POST到后端db中。

[上一级](../libsync.md)

[根目录](../../index.md)






