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
  const char* caPath = "/etc/ssl/certs/ca-certificates.crt";

  const std::string hostname = "api.binance.com";
  const int port = 443;
  const std::string proxyHostname = "127.0.0.1";
  const int proxyPort = 20112;

  netkit::Agent agent(hostname, port, caPath, proxyHostname, proxyPort);
  std::string url = "/api/v3/depth";
  std::unordered_map<std::string, std::string> params{{"symbol", "ETHUSDT"}};
  auto response = agent.request(url, params);

  std::cout << response << std::endl;
}