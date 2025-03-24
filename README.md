# traceroute

A simple traceroute utility implemented in C++ using raw sockets.

## Usage

```shell
./traceroute -h <max_hops> <destination>
```

**Note:** Running this utility requires root privileges due to the use of raw sockets.

## Compilation

This project uses CMake for building:

```shell
mkdir build && cd build
cmake ..
make
```