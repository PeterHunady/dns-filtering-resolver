// file: src/sockets.cpp

// Author: Peter Huňady
// Login: xhunadp00

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "sockets.hpp"

int listeningSocket(int port, bool verbose) {
    int lSocket = - 1;
    struct sockaddr_in6 serverAddress;

    if (verbose) {
        std::cout << "Creating listening socket...\n";
    }

    // Create UDP socket for DNS queries
    lSocket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (lSocket  < 0) {
        std::cerr << "Error: Listening socket failure.\n";
        exit(10);
    }

    int ipv6_only = 0;
    if (setsockopt(lSocket, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only)) < 0) {
        std::cerr << "Error: Failed to set IPV6_V6ONLY.\n";
        close(lSocket);
        exit(10);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr = in6addr_any;
    serverAddress.sin6_port = htons(port);

    // Bind to the port
    if (bind(lSocket, (const struct sockaddr *) &serverAddress,  sizeof(serverAddress)) < 0){
        std::cerr << "Error: Listening socket bind failure.\n";
        close(lSocket);
        exit(11);
    }

    // Timeout 1 second for graceful shutdown
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(lSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error: Failed to set listening socket timeout.\n";
        close(lSocket);
        exit(12);
    }

    if (verbose) {
        std::cout << "Listening socket succesfully created.\n\n";
    }

    return lSocket;
}

int resolvingSocket(const std::string& server, int port, bool verbose) {
    int rSocket = -1;
    struct addrinfo hints;
    struct addrinfo *serverInfo;
    struct addrinfo *p;

    if (verbose) {
        std::cout << "Creating resolving socket...\n";
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(server.c_str(), std::to_string(port).c_str(), &hints, &serverInfo) != 0) {
        std::cerr << "Error: Invalid DNS server name.\n";
        exit(13);
    }

    // Check all endpoints to connect
    for (p = serverInfo; p != NULL; p = p->ai_next) {
        rSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (rSocket == -1) {
            if (verbose) {
                std::cerr << "Resolving socket creation failed.\n";
            }
            continue;
        }

        // Connect to server
        if (connect(rSocket, p->ai_addr, p->ai_addrlen) == -1) {
            if (verbose) {
                std::cerr << "Resolving socket connect failed.\n";
            }
            close(rSocket);
            rSocket = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(serverInfo);

    if (rSocket < 0) {
        std::cerr << "Error: Failed to connect to server.\n";
        exit(14);
    }

    // Set receive timeout to 2 seconds
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (setsockopt(rSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error: Failed to set resolving socket timeout.\n";
        close(rSocket);
        exit(15);
    }

    if (verbose) {
        std::cout << "Resolving socket succesfully created.\n\n";
    }

    return rSocket;
}
