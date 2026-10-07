// file: dns.cpp

// Author: Peter Huňady
// Login: xhunadp00

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <csignal>

#include "src/sockets.hpp"
#include "src/filter.hpp"
#include "src/dnsPacket.hpp"
#include "src/parameters.hpp"

struct ThreadData {
  int lSocket;
  std::string server;
  bool verbose;
  pthread_mutex_t* sendMutex;
  struct sockaddr_in6 sourceAddress;
  std::vector<char> queryBuffer;
  std::string domainName;
};


struct ThreadData;
void handleQuery(ThreadData data);
void* handleQueryWrapper(void* arg);
void cleanupGlobalResources();


volatile sig_atomic_t isRunning = 1;
std::vector<std::string>* tryFilteredDomains = nullptr;
Parameters* tryParameters = nullptr;
int trylSocket = -1;
bool isCleanedUp = false;

void signal_handler(int signum) {
    isRunning = 0;
}

void cleanupGlobalResources() {
    if (isCleanedUp) return;
    isCleanedUp = true;

    if (tryFilteredDomains) {
        delete tryFilteredDomains;
        tryFilteredDomains = nullptr;
    }
    if (tryParameters) {
        delete tryParameters;
        tryParameters = nullptr;
    }
    if (trylSocket >= 0) {
        close(trylSocket);
        trylSocket = -1;
    }
}


int main(int argc, char* argv[]) {
  tryFilteredDomains = new std::vector<std::string>();
  tryParameters = new Parameters();

  atexit(cleanupGlobalResources);
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  Parameters &parameters = *tryParameters;
  parseParameters(argc, argv, parameters);

  // Load all filtered domains from filter file
  *tryFilteredDomains = getAllDomains(parameters.filterFile, parameters.verbose);
  std::vector<std::string> &filteredDomains = *tryFilteredDomains;

  // UDP listening socket
  trylSocket = listeningSocket(parameters.port, parameters.verbose);
  int lSocket = trylSocket;

  // Validate DNS server by creating and closing a test socket
  int tryrSocket = resolvingSocket(parameters.server, 53, parameters.verbose);
  close(tryrSocket);

  pthread_mutex_t sendMutex;
  pthread_mutex_init(&sendMutex, nullptr);

  std::vector<pthread_t> threads;

  char buffer [1024];
  struct sockaddr_in6 sourceAddress;
  socklen_t sourceAddressLength;

  if (parameters.verbose){
    std::cout << "Starting communication...\n\n";
  }

  while(isRunning) {
    sourceAddressLength = sizeof(sourceAddress);

    // Receive a client querry
    int receive = recvfrom(lSocket, buffer, 1024, 0, (struct sockaddr*) &sourceAddress, &sourceAddressLength);
    if (receive < 0) {
      if (!isRunning) {
        break;
      }
      continue;
    }

    std::string domainName;

     // Check DNS packet and filter
    int response = checkDnsPacket(lSocket, buffer, filteredDomains, size_t(receive), sourceAddress, parameters.verbose, domainName);
    if (response != 0){
      continue;
    }

    if (parameters.verbose) {
      std::cout << "\nQuery for '" << domainName << "' accepted, forwarding to DNS server...\n\n";
    }

    // Create thread data structure
    ThreadData* threadData = new ThreadData();
    threadData->lSocket = lSocket;
    threadData->server = parameters.server;
    threadData->verbose = parameters.verbose;
    threadData->sendMutex = &sendMutex;
    threadData->sourceAddress = sourceAddress;
    threadData->queryBuffer = std::vector<char>(buffer, buffer + receive);
    threadData->domainName = domainName;

    // Parallel query processing
    pthread_t queryThread;
    pthread_create(&queryThread, nullptr, handleQueryWrapper, threadData);
    threads.push_back(queryThread);
  }

  // Clean up threads
  for (size_t i = 0; i < threads.size(); i++) {
    pthread_join(threads[i], nullptr);
  }

  // Clean up
  pthread_mutex_destroy(&sendMutex);
  cleanupGlobalResources();

  return 0;
}

void handleQuery(ThreadData data) {
  int rSocket = resolvingSocket(data.server, 53, false);

  const int maxRetries = 3;
  bool success = false;

  for (int attempt = 1; attempt <= maxRetries && !success; attempt++) {
    if (data.verbose && attempt > 1) {
      std::cout << "\nQuery for '" << data.domainName << "': Retry attempt " << attempt << " of " << maxRetries << ".\n";
    }

    // Send query to DNS server
    if (send(rSocket, data.queryBuffer.data(), data.queryBuffer.size(), 0) < 0) {
      if (data.verbose) {
        std::cerr << "\nQuery for '" << data.domainName << "': Error sending to DNS server (attempt " << attempt << ").\n";
      }
      continue;
    }

    char responseBuffer[1024];

    // Receive response from DNS server
    int received = recv(rSocket, responseBuffer, 1024, 0);

    if (received > 0) {
      pthread_mutex_lock(data.sendMutex);
      // Send response to client
      sendto(data.lSocket, responseBuffer, received, 0, (const struct sockaddr*) &data.sourceAddress, sizeof(data.sourceAddress));
      pthread_mutex_unlock(data.sendMutex);

      if (data.verbose) {
        std::cout << "\nQuery for '" << data.domainName << "': Successfully forwarded response to client.\n\n";
      }
      success = true;

    }else if (data.verbose) {
      if (attempt < maxRetries) {
        std::cerr << "\nQuery for '" << data.domainName << "': Timeout or error receiving response (attempt " << attempt << "), retrying...\n";
      }else {
        std::cerr << "\nQuery for '" << data.domainName << "': FAILED after " << maxRetries << " attempts.\n\n";
      }
    }
  }

  // Fail, send SERVFAIL to client
  if (!success) {
    pthread_mutex_lock(data.sendMutex);
    sendErrorPacket(data.lSocket, data.queryBuffer.data(), data.queryBuffer.size(), data.sourceAddress, 2);
    pthread_mutex_unlock(data.sendMutex);

    if (data.verbose) {
      std::cerr << "\nQuery for '" << data.domainName << "': Sent SERVFAIL (RCODE 2) to client.\n\n";
    }
  }

  // Close socket
  close(rSocket);
}

void* handleQueryWrapper(void* arg) {
  ThreadData* data = static_cast<ThreadData*>(arg);
  handleQuery(*data);
  delete data;
  return nullptr;
}