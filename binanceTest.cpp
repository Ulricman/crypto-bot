#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "cexkit/binance/datahub.hpp"

using json = nlohmann::json;

void depthCB(netkit::Frame frame) {
  nlohmann::json payload = nlohmann::json::parse(frame.payload);
  std::cout << "CB[" << payload["stream"] << "]\n";
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
  const std::string proxyHostname = config["proxy_host"];
  const int proxyPort = config["proxy_port"];

  const std::string hostname = "stream.binance.com";
  const int port = 9443;

  const std::string endpoint = "/stream";
  cexkit::binance::DataHub datahub(hostname, port, caPath, apiKey, secretKey,
                                   endpoint, proxyHostname, proxyPort);
  std::cout << "ws connected\n";
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // datahub.subscribe("solusdt@depth@100ms");
  // datahub.registerCallback("solusdt@depth@100ms", depthCB);
  // datahub.subscribe("ethusdt@depth@100ms");
  // datahub.registerCallback("ethusdt@depth@100ms", depthCB);

  // std::this_thread::sleep_for(std::chrono::seconds(10));
  // datahub.unsubscribe("solusdt@depth@100ms");
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // datahub.subscribe("btcusdt@depth@100ms");
  // datahub.registerCallback("btcusdt@depth@100ms", depthCB);
  datahub.localOrderBook("solusdt");
  std::this_thread::sleep_for(std::chrono::seconds(200));
}