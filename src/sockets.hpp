// file: src/sockets.hpp

// Author: Peter Huňady
// Login: xhunadp00

#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <string>
#include <netdb.h> 

int listeningSocket(int port, bool verbose);

int resolvingSocket(const std::string& server, int port, bool verbose);

#endif