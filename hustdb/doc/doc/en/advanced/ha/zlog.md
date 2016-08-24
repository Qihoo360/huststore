zlog 配置文件
--

路径：`hustdb/ha/nginx/conf/zlog.conf`

### 配置范例 ###

以下是一个完整的配置文件：

    [global]
	strict init = true
	buffer min = 2MB
	buffer max = 64MB
	rotate lock file = /tmp/zlog.lock
	file perms = 755
	
	[formats]
	default = "[%d] [%V] | %m%n"
	
	[rules]
	*.*             "/data/hustdbha/logs/%M(sync_dir)/%d(%Y-%m-%d-%H-%M).log"; default


**以下字段根据实际生产环境配置合适的值：**

* `rules`: 日志规则  
样例中 `/data/hustdbha` 代表安装目录，实际使用时替换为真实的路径即可。

**除此之外的其他字段均建议保持默认值**。

[上一级](conf.md)

[根目录](../../index.md)