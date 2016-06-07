FAQ
--

本节提供 `libsync` 部分问题的解答

**Q:**	`libsync` 有多少线程？

**A:**	 `4+n` 个；包括 `主线程`，`check_backend&release_file` ， `monitor` ， `read_log` 等4个线程，以及线程池的n个工作线程 

**Q:** `libsync` 模块各子模块间如何配合？

**A:** 子模块间通过 `pipe` 或者 `queue` 配合

**Q:** `libsync` 的工作模式？

**A:**  由 `nginx` 的 `master`进程动态加载 `libsync.so`

**Q:** `libsync`的局限与风险？

**A:**
  
* 由于 `libsync` 运行于 `nginx` 的 `master` 进程空间，如果 `libsync` 出现bug，可能导致 `master` 进程崩溃。

* 对于 `libsync` 开启前生成的日志， `libsync`无法自动同步。如果要同步这些日志，需要手动重新拷贝到 `logs`目录下对应的 `backends` 目录中。
		
		

[上一级](index.md)

[根目录](../../index.md)  