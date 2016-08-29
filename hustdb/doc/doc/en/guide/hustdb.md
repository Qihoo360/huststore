hustdb
--

Install `hustdb`(need sudo, used for libsnappy, libevhtp, libevent2.0): 

    $ cd hustdb/db/server/make/linux/
    $ sh build.sh

Target path

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

Start service

    $ cd hustdb/db/server/make/linux/
    $ ./hustdb

Type in the below command to test:

    curl -i -X GET 'localhost:8085/status.html'

Infomation returned:

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

The result shows that the servers work as expected

[Previous page](index.md)

[Home](../index.md)