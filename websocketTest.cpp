#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "netkit/websocket.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

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
  const std::string proxyHostname = config["proxy_host"];
  const int proxyPort = config["proxy_port"];

  const std::string hostname = "stream.binance.com";
  const int port = 9443;
  const std::string endpoint = "/stream";

  netkit::Websocket websocket(hostname, port, caPath, apiKey, secretKey,
                              endpoint, proxyHostname, proxyPort);

  const std::string stream = "btcusdt@depth@100ms";
  // std::vector<std::string> streams{"btcusdt@depth", "ethusdt@depth"};
  websocket.subscribe(stream);
}