数据同步服务
--

`sync server` 用于将本地日志同步到后端 `db` 中，主要包含 `libsync` 和 `network` 两个模块。

说明：**sync server 不是必须安装的模块**。如果不部署该服务，HA的如下接口将失效：

* [sync_status](../../api/ha/sync_status.md)
* [sync_alive](../../api/ha/sync_alive.md)

同时，如果HA和DB之间的网络情况不理想，**数据不一致的概率会增加**。

**请根据生产环境的实际情况来决定是否部署该服务**。

### libsync ###

目录：`hustdb/sync/module`
#### `check_backend` ####

功能：定时检测后端db存活。

如果db可用，则通过 `pipe` 通知 `read_log` 模块读取该 `db` 下的日志文件，开始同步。

#### `release_file` ####

功能： 定时清理同步完成的文件

通过位图检测 `release_queue` 中的文件是否同步完成，如果同步完成则删除文件并释放资源。

#### `monitor` ####

功能：监控日志文件目录。

如果产生日志文件，则将文件加入对应 `db` 的 `file_queue`。

#### `read_log` ####

功能：读取日志文件。

从 `file_queue` 中取出日志文件，按行读取该日志文件，将数据加入线程池的 `data_queue`。

将读取完成的文件加入到 `release_queue` 中。

#### `sync_threadpool` ####

功能：线程池，同步数据。

线程池中线程从 `data_queue` 中取出数据，进行 `base64` 解码，构造 `url`，`query` 等，`POST` 到后端 `db` 中。

### network ###

目录：`hustdb/sync/network`

该模块 **仅对 `HA` 模块提供 `http` 查询服务**。

目前提供的接口包括如下两个：

#### `status` ####

**接口:** `/status.html`

**方法:** `GET`

**参数:** 无

该接口用于判断 `sync server` 是否存活。  
与 `HA` 对应的接口： [sync_alive](../../api/ha/sync_alive.md)

#### `sync_status` ####

**接口:** `/sync_status`

**方法:** `GET`

**参数:** 

*  **backend_count** （必选）  

该接口用于获取 `sync server` 进行数据同步的实时状态。  
与 `HA` 对应的接口： [sync_status](../../api/ha/sync_status.md)


### 相关问题 ###

**Q:**	`sync server` 有多少线程？  
**A:**	 `4+n` 个；包括 `主线程`，`check_backend&release_file` ， `monitor` ， `read_log` 等4个线程，以及线程池的n个工作线程 

**Q:** `sync server` 模块各子模块间如何配合？  
**A:** 子模块间通过 `pipe` 或者 `queue` 配合

**Q:** `sync server` 于 `HA` 如何通信？  
**A:**  以 `libevhtp` 为基本框架封装成 `http` 服务，`HA` 通过子请求查询 `sync server` 提供的接口。

**Q:** `sync server` 的局限与风险？  
**A:**  对于 `sync server` 开启前生成的日志， `sync server`无法自动同步。如果要同步这些日志，需要手动重新拷贝到 `logs`目录下对应的 `backends` 目录中。

[上一页](../ha.md)

[回首页](../../index.md)






