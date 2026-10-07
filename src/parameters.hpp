// file: src/parameters.hpp

// Author: Peter Huňady
// Login: xhunadp00

#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <string>

struct Parameters {
  std::string server;
  int port = 53;
  std::string filterFile;
  bool verbose = false;
};

void parseParameters(int argc, char* argv[], Parameters& parameters);
void help();

#endif