// file: src/filter.cpp

// Author: Peter Huňady
// Login: xhunadp00

#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

std::vector<std::string> getAllDomains(const std::string& fileName, bool verbose) {
    if (verbose) {
        std::cout << "Trying to open filter file...\n";
    }

    std::fstream file;

    file.open(fileName, std::ios::in);

    if (!file.is_open()) {
        std::cerr << "Error: Opening a file failed.\n";
        exit(8);
    }

    if (verbose) {
        std::cout << "Filter file opened succesfully.\n";
    }

    std::vector<std::string> domains;
    std::string domainName;

    if (verbose) {
        std::cout << "Loading domains from filter file...\n";
    }
    while(getline(file, domainName)) {
        // Trim
        while (!domainName.empty() && (domainName[0] == ' ' || domainName[0] == '\t' || domainName[0] == '\r' || domainName[0] == '\n')) {
            domainName.erase(0, 1);
        }

        while (!domainName.empty() && (domainName[domainName.length()-1] == ' ' || domainName[domainName.length()-1] == '\t' || domainName[domainName.length()-1] == '\r' || domainName[domainName.length()-1] == '\n')) {
            domainName.erase(domainName.length()-1, 1);
        }

        // Skip comments and empty lines
        if (domainName.rfind("#", 0) == 0 || domainName == "") {
            continue;
        }

        // Transform to lowercase
        for (int i = 0; i < (int)domainName.length(); i++) {
            domainName[i] = std::tolower(domainName[i]);
        }

        domains.push_back(domainName);
    }

    file.close();

    // Check if filter file is empty
    if (domains.empty()){
        std::cerr << "Error: Empty filter file.\n";
        exit(9);
    }

    if (verbose) {
        std::cout << "Domains successfully loaded from filter file.\n\n";
    }

    return domains;
}