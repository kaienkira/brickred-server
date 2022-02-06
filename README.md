brickred-server
===============
brickred c++ server library

Features
--------
* reactor network io
* multithread support
* min-heap based timer
* log util
* http protocol support
* websocket protocol support

Platform support
----------------
only linux, maybe port to other platforms (unix likes) in the future

Compile
-------
```
$ config.sh [--prefix=<prefix>] [--build-test]
$ make && make install
```
