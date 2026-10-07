// file: src/parameters.cpp

// Author: Peter Huňady
// Login: xhunadp00

#include <iostream>
#include <string>
#include <cstdlib>
#include "parameters.hpp"

void parseParameters(int argc, char* argv[], Parameters& parameters){
  if (argc == 1) {
    help();
  }

  // Track if parameters were already set
  bool serverSet = false;
  bool portSet = false;
  bool filterSet = false;

  // Parse parameters to struct Parameters
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      help();
    }

    else if (arg == "-s") {
      if (serverSet) {
        std::cerr << "Error: Server parameter (-s) already set.\n";
        exit(7);
      }

      if (i + 1 < argc) {
        parameters.server = argv[++i];

        serverSet = true;
        continue;
      }

      std::cerr << "Error: Missing server name.\n";
      exit(2);
    }

    else if (arg == "-p") {
      if (portSet) {
        std::cerr << "Error: Port parameter (-p) already set.\n";
        exit(7);
      }

      if (i + 1 < argc) {
        parameters.port = std::stoi(argv[++i]);
        if (parameters.port < 0 || parameters.port > 65535) {
          std::cerr << "Error: Invalid port number. Port must be in range 0-65535.\n";
          exit(3);
        }
        portSet = true;
        continue;
      }

      std::cerr << "Error: Missing port number.\n";
      exit(3);
    }

    else if (arg == "-f") {
      if (filterSet) {
        std::cerr << "Error: Filter file parameter (-f) already set.\n";
        exit(7);
      }

      if (i + 1 < argc) {
        parameters.filterFile = argv[++i];

        filterSet = true;
        continue;
      }

      std::cerr << "Error: Missing name of filter file.\n";
      exit(4);
    }

    else if (arg == "-v") {
      parameters.verbose = true;
    }

    else {
      std::cerr << "Error: Invalid parameter.\n";
      exit(1);
    }
  }

  if (parameters.verbose) {
    std::cout << "Server: " << parameters.server << "\n"
            << "Port: " << parameters.port << "\n"
            << "Filter file: " << parameters.filterFile << "\n\n";
  }

  if (parameters.server.empty()) {
    std::cerr << "Error: Parameter server is required.\n";
    exit(5);
  }

  if (parameters.filterFile.empty()) {
    std::cerr << "Error: Parameter file name is required.\n";
    exit(6);
  }

  // Transform to lowercase
  for (int i = 0; i < (int)parameters.server.length(); ++i) {
    parameters.server[i] = std::tolower(parameters.server[i]);
  }

  if (parameters.verbose) {
    std::cout << "Paramaters successfully parsed.\n";
  }
}

void help(){
  std::cout << "Filtering DNS resolver\n"
            << "Usage: dns -s server [-p port] -f filter file\n\n"
            << "-s server: IP address or domain name of DNS server.\n"
            << "-p port: The port number on which the program will listen for queries. Default port is 53.\n"
            << "-f filter file: name of the file contaning unwanted domains\n"
            << "-v: print resolving informations.\n"
            << "-h | --help: print this message\n\n";
   exit(0);
}