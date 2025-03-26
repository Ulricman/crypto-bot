#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "netkit/proxy.hpp"
#include "netkit/rest.hpp"
#include "netkit/utils.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

void readConfig(const char* configPath) {
  std::ifstream file(configPath);
  json config;
  file >> config;
}

int main() {
  // Read configs.
  const char* configPath = "/home/jeffrey/crypto-bot/config.json";
  std::ifstream file(configPath);
  json config;
  file >> config;
  file.close();

  const std::string caPath = config["ca_path"];
  const std::string apiKey = config["api_key"];
  const std::string secretKey = config["api_secret"];

  const std::string hostname = "api.binance.com";
  const int port = 443;
  const std::string proxyHostname = config["proxy_host"];
  const int proxyPort = config["proxy_port"];

  // Main parts.
  netkit::Rest rest(hostname, port, caPath.data(), apiKey, secretKey,
                    proxyHostname, proxyPort);

  // std::cout << agent.sendPublicRequest("/api/v3/depth", {{"symbol",
  // "SOLUSDT"}})
  //           << std::endl;
  std::cout << rest.sendSignedRequest("/api/v3/account", "GET",
                                      {{"omitZeroBalances", "true"}})
            << std::endl;
  // std::cout << agent.sendPublicRequest("/api/v3/ticker/bookTicker", "GET",
  //                                      {{"symbol", "BTCUSDT"}})
  //           << std::endl;
}