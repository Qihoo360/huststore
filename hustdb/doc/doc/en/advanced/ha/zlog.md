zlog Configuration
--

Path: `hustdb/ha/nginx/conf/zlog.conf`

### Configuration Example ###

A complete configuration file example:

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


**Set a suitable value for the below fields in your specific network environment**

* `rules`: Log rules
In the above example script, `/data/hustdbha` is installation path, replace it with your real path in your production environment.

**In exception of this, use default values for other fields.**

[Previous](conf.md)

[Home](../../index.md)