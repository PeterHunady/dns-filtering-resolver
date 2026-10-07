// file: src/dnsPacket.cpp

// Author: Peter Huňady
// Login: xhunadp00

#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdint>
#include <algorithm>
#include <vector>

struct dnsHeader{
    unsigned id :16;

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        unsigned qr :1;
        unsigned opcode :4;
        unsigned aa :1;
        unsigned tc :1;
        unsigned rd :1;
        unsigned ra :1;
        unsigned z :3;
        unsigned rcode :4;
    #else
        unsigned rd :1;
        unsigned tc :1;
        unsigned aa :1;
        unsigned opcode :4;
        unsigned qr :1;
        unsigned rcode :4;
        unsigned z :3;
        unsigned ra :1;
    #endif

    unsigned qdcount :16;
    unsigned ancount :16;
    unsigned nscount :16;
    unsigned arcount :16;
};

struct dnsQuestion{
    unsigned qtype :16;
    unsigned qclass :16;
};


void sendErrorPacket(int lSocket, char* buffer, int receive, struct sockaddr_in6 sourceAddress, int rcode) {
    dnsHeader *header = (dnsHeader *)buffer;

    header->qr = 1;
    header->rcode = rcode;

    // Clean flags
    header->aa = 0;
    header->tc = 0;
    header->z = 0;
    header->ra = 0;
    header->ancount = 0;
    header->nscount = 0;

    // Send message to server
    sendto(lSocket, header, receive, 0, (const struct sockaddr *) &sourceAddress, sizeof(sourceAddress));
}

int checkDnsPacket(int lSocket, char* buffer, std::vector<std::string> filteredDomains, size_t length, struct sockaddr_in6 sourceAddress, bool verbose, std::string& domainName){
    if (verbose) {
        std::cout << "Received DNS query, checking packet...\n";
    }

    if (length < sizeof(dnsHeader)) {
        if (verbose) {
            std::cerr << "Error: Packet too short (RCODE 1).\n";
        }
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 1);
        return 1;
    }

    dnsHeader *header = (dnsHeader *) buffer;

    // Check if it is a query or
    if (header->qr != 0 || header->opcode != 0) {
        if (verbose) {
            std::cerr << "Error: Invalid query format (RCODE 5).\n";
        }
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 5);
        return 5;
    }

    uint16_t qdcount = ntohs(header->qdcount);


    // Check if it is only one question
    if (qdcount == 0) {
        if (verbose) {
            std::cerr << "Error: No questions in packet (RCODE 1).\n";
        }
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 1);
        return 1;
    }

    if (qdcount != 1) {
        if (verbose) {
            std::cerr << "Error: Multiple questions not supported (RCODE 4).\n";
        }
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 4);
        return 4;
    }

    if (verbose) {
        std::cout << "Parsing domain name from query...\n";
    }

    size_t offset = sizeof(dnsHeader);
    domainName = "";

    // Parsing DNS domain name
    while (offset < length) {
        unsigned char labelLength = (unsigned char)buffer[offset++];
        if (labelLength == 0) break;

        if (offset + labelLength > length) {
            sendErrorPacket(lSocket, buffer, length, sourceAddress, 1);
            return 1;
        }

        if (!domainName.empty()) {
            domainName += '.';
        }

        domainName.append(&buffer[offset], &buffer[offset] + labelLength);
        offset += labelLength;
    }

    // Trim
    while (!domainName.empty() && (domainName[0] == ' ' || domainName[0] == '\t' || domainName[0] == '\r' || domainName[0] == '\n')) {
        domainName.erase(0, 1);
    }
    while (!domainName.empty() && (domainName[domainName.length()-1] == ' ' || domainName[domainName.length()-1] == '\t' || domainName[domainName.length()-1] == '\r' || domainName[domainName.length()-1] == '\n')) {
        domainName.erase(domainName.length()-1, 1);
    }

    if (offset + sizeof(dnsQuestion) > length) {
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 1);
        return 1;
    }

    dnsQuestion *question = (dnsQuestion * ) &buffer[offset];
    uint16_t qtype = ntohs(question->qtype);
    uint16_t qclass = ntohs(question->qclass);
    offset += sizeof(dnsQuestion);

    // Check for A qtype
    if (qtype != 1 || qclass != 1) {
        sendErrorPacket(lSocket, buffer, length, sourceAddress, 4);
        return 4;
    }
    
    if (verbose) {
        std::cout << "Query: " << domainName << "\n";
        std::cout << "DNS packet is valid.\n";
        std::cout << "Checking if '" << domainName << "' is in filter list...\n";
    }

    // Transform to lowercase
    for (int i = 0; i < (int)domainName.length(); i++) {
        domainName[i] = std::tolower(domainName[i]);
    }

    // Check if requested domain is in filteredDomains
    for (int i = 0; i < (int)filteredDomains.size(); i++){
        std::string filteredDomain = filteredDomains[i];

        if (domainName == filteredDomain) {
            if (verbose) {
                std::cerr << "\nQuery for '" << domainName << "' BLOCKED: Domain is in filter list (RCODE 5).\n";
            }
            sendErrorPacket(lSocket, buffer, length, sourceAddress, 5);
            return 5;
        }

        if (domainName.size() > filteredDomain.size()) {
            int position = domainName.size() - filteredDomain.size();
            if (position > 0 && domainName[position - 1] == '.' && domainName.compare(position, std::string::npos, filteredDomain) == 0) {
                if (verbose) {
                    std::cerr << "\nQuery for '" << domainName << "' BLOCKED: Subdomain of filtered domain '" << filteredDomain << "' (RCODE 5).\n";
                }
                sendErrorPacket(lSocket, buffer, length, sourceAddress, 5);
                return 5;
            }
        }
    }

    if (verbose) {
        std::cout << "Domain '" << domainName << "' is NOT in filter list - allowing query.\n";
    }

    return 0;
}