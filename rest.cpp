#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "netKit/https.hpp"
#include "netKit/proxy.hpp"
#include "netKit/utils.hpp"

int main() {
  // Basic settings.
  const char* caPath = "/etc/ssl/certs/ca-certificates.crt";
  const std::string apiKey =
      "KiuKcWHgGD16UkX5JOS6hdkkl84fxHkKnKSA6j16bGnm0qHFsdTbK3fZoKnGJ0pj";
  const std::string secretKey =
      "7nz1JfEHEZ2GoNuLemwBXirEEpwZtYMgpmw75XlaKTBeBMzfmYevH92Bvtdygiwn";

  const std::string hostname = "api.binance.com";
  const int port = 443;
  const std::string proxyHostname = "127.0.0.1";
  const int proxyPort = 20112;

  // Main parts.
  netkit::Agent agent(hostname, port, caPath, apiKey, secretKey, proxyHostname,
                      proxyPort);

  // std::cout << agent.sendPublicRequest("/api/v3/depth", {{"symbol",
  // "SOLUSDT"}})
  //           << std::endl;
  std::cout << agent.sendSignedRequest("/api/v3/account", "GET",
                                       {{"omitZeroBalances", "true"}})
            << std::endl;
  // std::cout << agent.sendPublicRequest("/api/v3/ticker/bookTicker", "GET",
  //                                      {{"symbol", "BTCUSDT"}})
  //           << std::endl;
}