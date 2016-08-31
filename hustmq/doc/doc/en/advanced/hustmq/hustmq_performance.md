hustmq performance
--

**Machine Configuration:** `24core, 64gb, 1tb sata(7200rpm)`

**Stress Test Parameters:** `100 concurrent, 1000 thousand querys, single queue`

**DB Configuration:** `single instance, thread model, 10 workers`

**Benchmark Result:**

    (1) PUT
	    <1>item : 256B;      qps : 30 thousand
	    <2>item : 1KB;       qps : 25 thousand
	    <3>item : 4KB;       qps : 20 thousand
	    <4>item : 16KB;      qps : 7 thousand
	    <5>item : 64KB;      qps : 2 thousand

	(2) GET
	    <1>item : 256B;      qps : 25 thousand
	    <2>item : 1KB;       qps : 20 thousand
	    <3>item : 4KB;       qps : 18 thousand
	    <4>item : 16KB;      qps : 7 thousand
	    <5>item : 64KB;      qps : 2 thousand

[Previous](../index.md)

[Home](../../index.md)

