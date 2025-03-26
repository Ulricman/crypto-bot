#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "cexkit/binance/datahub.hpp"

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

  cexkit::binance::DataHub datahub(hostname, port, caPath, apiKey, secretKey,
                                   proxyHostname, proxyPort);
  datahub.subscribe("solusdt@depth@100ms");
  std::this_thread::sleep_for(std::chrono::seconds(300));
  datahub.unsubscribe("solusdt@depth@100ms");

  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    datahub.listSubscriptopns();
  }
}