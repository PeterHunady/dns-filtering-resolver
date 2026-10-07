# DNS Filtering Resolver

A multithreaded DNS filtering resolver written in C++ for Linux. The resolver accepts DNS **A** queries over UDP, blocks configured domains, forwards allowed queries to an upstream DNS server, and returns the upstream response to the client.

This repository is based on a university networking project and is published as a code sample.

## Features

- UDP-based DNS resolver
- DNS A-query parsing and validation
- Domain filtering from a configurable file
- `REFUSED` responses for blocked domains
- Forwarding of allowed queries to an upstream DNS server
- Retransmission and timeout handling
- `SERVFAIL` response after repeated upstream failures
- Multithreaded request processing using POSIX threads
- Mutex-protected writes to the client-facing socket
- Verbose mode for debugging
- Automated test script

## Tech stack

- C++17
- POSIX sockets
- pthreads
- Linux
- Make

## Build

```bash
make
```

The build creates the `dns` executable.

## Usage

Display help:

```bash
./dns --help
```

The resolver expects an upstream DNS server, a listening port, and a file containing domains to block. Run `./dns --help` for the exact command-line options supported by the implementation.

Example filter file:

```text
facebook.com
example.com
# lines starting with # are ignored
```

## How it works

1. The main thread listens for DNS queries on a UDP socket.
2. Incoming DNS packets are parsed and validated.
3. If the requested domain matches the filter list, the resolver returns a DNS `REFUSED` response.
4. Otherwise, the request is forwarded to the configured upstream DNS server.
5. Each accepted request is handled in its own worker thread.
6. If the upstream server does not respond, the resolver retries the request and eventually returns `SERVFAIL`.

## Project structure

```text
.
├── dns.cpp
├── src/
│   ├── dnsPacket.cpp/.hpp
│   ├── filter.cpp/.hpp
│   ├── parameters.cpp/.hpp
│   └── sockets.cpp/.hpp
├── tests/
│   └── tests.sh
├── Makefile
├── docs/
└── LICENSE
```

## Limitations

- UDP only
- DNS A queries only
- Intended for Linux/POSIX environments

## Testing

```bash
make test
```

## License

MIT License. See [LICENSE](LICENSE).
