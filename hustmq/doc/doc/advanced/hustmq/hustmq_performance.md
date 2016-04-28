hustmq performance
--

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，100w query，single queue`

**DB CONF:** `single instance，thread model，10 worker`

**测试结果:**

    （1）PUT
	    <1>item：256B；     qps：3w
	    <2>item：1KB；      qps：2.5w
	    <3>item：4KB；      qps：2w
	    <4>item：16KB；     qps：7k
	    <5>item：64KB；     qps：2k

	（2）GET
	    <1>item：256B；     qps：2.5w
	    <2>item：1KB；      qps：2w
	    <3>item：4KB；      qps：1.8w
	    <4>item：16KB；     qps：7k
	    <5>item：64KB；     qps：2k

	（3）STAT_ALL           qps：9.5w

[上一级](../index.md)

[根目录](../../index.md)

