# opendlv-proxy

This repository provides the source code to a UDP tunnel for bridging between 
conferences, possibly with different CIDs and on different networks.


## Usage
This microservice is created automatically on new releases, via Docker's public registry:

* [x86_64](https://hub.docker.com/r/chalmersrevere/opendlv-proxy-amd64/tags/)
* [armhf](https://hub.docker.com/r/chalmersrevere/opendlv-proxy-armhf/tags/)

To run this microservice using our pre-built Docker multi-arch images, 
simply start it as in the following example. In a network, two OD4 conferences are running, one
with the CID 111 on a machine with the IP 192.168.0.1, and one with the CID 112
on a machine with the IP 192.168.0.2. To bridge the two conferences the following
two instances of opendlv-proxy are started:

```
docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=111 --target-ip=192.168.0.2 --target-port=10000 --port=10000

docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=112 --target-ip=192.168.0.1 --target-port=10000 --port=10000
```

It is also possible to combine several external conferences (on computers with IPs 192.168.0.2
and 192.168.0.3) into one central conference (on a computer with IP 192.168.0.1)
by running the following setup:
```
docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=111 --target-ip=192.168.0.1 --target-port=10001 --port=10000

docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=112 --target-ip=192.168.0.1 --target-port=10002 --port=10000

docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=115 --target-ip=192.168.0.2 --target-port=10000 --port=10001 --sender-stamp-offset=100

docker run --init --rm --net=host chalmersrevere/opendlv-proxy-amd64:v0.0.1 opendlv-proxy --cid=115 --target-ip=192.168.0.3 --target-port=10000 --port=10002 --sender-stamp-offset=200
```
The --sender-stamp-offset flag separates the message streams by appending the offset
to any received message on that link. Furthermore, this feature also filters messages
based on sender stamps, so in order to send a message to the external conference with
CID 111, a message can be sent by adding 100 to the sender stamp.

## Build from sources
To build this software, you need cmake, C++14 or newer, and make. Having these
preconditions, just run `cmake` and `make` as follows:

```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License
