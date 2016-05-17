hustdb performance
--

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，1000 thousand querys`

**DB CONF:** `single instance，thread model，10 workers`

**测试结果:**

    （1）PUT
    	<1>value：256B；     qps：95 thousand
	    <2>value：1KB；      qps：85 thousand
	    <3>value：4KB；      qps：25 thousand
	    <4>value：16KB；     qps：7 thousand
	    <5>value：64KB；     qps：2 thousand

	（2）GET
	    <1>value：256B；     qps：100 thousand
	    <2>value：1KB；      qps：10 thousand
	    <3>value：4KB；      qps：25 thousand
	    <4>value：16KB；     qps：7 thousand
	    <5>value：64KB；     qps：2 thousand

	（3）DEL
    	<1>value：256B；     qps：100 thousand
	    <2>value：1KB；      qps：100 thousand
    	<3>value：4KB；      qps：100 thousand
    	<4>value：16KB；     qps：100 thousand
    	<5>value：64KB；     qps：100 thousand

[上一级](index.md)

[根目录](../index.md)
