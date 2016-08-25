hustmq performance
--

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，1000 thousand querys，single queue`

**DB CONF:** `single instance，thread model，10 workers`

**测试结果:**

    （1）PUT
	    <1>item：256B；     qps：30 thousand
	    <2>item：1KB；      qps：25 thousand
	    <3>item：4KB；      qps：20 thousand
	    <4>item：16KB；     qps：7 thousand
	    <5>item：64KB；     qps：2 thousand

	（2）GET
	    <1>item：256B；     qps：25 thousand
	    <2>item：1KB；      qps：20 thousand
	    <3>item：4KB；      qps：18 thousand
	    <4>item：16KB；     qps：7 thousand
	    <5>item：64KB；     qps：2 thousand

[上一级](../index.md)

[根目录](../../index.md)

