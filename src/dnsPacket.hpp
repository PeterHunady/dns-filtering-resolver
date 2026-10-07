// file: src/dnsPacket.hpp

// Author: Peter Huňady
// Login: xhunadp00

#ifndef CREATEDNSPACKET_HPP
#define CREATEDNSPACKET_HPP

#include <vector>
#include <string>
#include <netinet/in.h>

void sendErrorPacket(int lSocket, char* buffer, int receive, struct sockaddr_in6 sourceAddress, int rcode);
int checkDnsPacket(int lSocket, char* buffer, std::vector<std::string> filteredDomains, size_t length, struct sockaddr_in6 sourceAddress, bool verbose, std::string& domainName);

#endif