#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "cexkit/binance/broker.hpp"
#include "cexkit/binance/datahub.hpp"

int main() {
  // Read configs.
  const char* configPath = "/home/jeffrey/crypto-kit/config.json";
  std::ifstream file(configPath);
  // json configFile;
  netkit::Config config;
  file >> config;
  file.close();

  const std::string wsHostname = "data-stream.binance.vision";
  const int wsPort = 9443;
  const std::string restHostname = "api.binance.com";
  const int restPort = 443;

  const std::string endpoint = "/stream";
  netkit::Rest rest(restHostname, restPort, config);
  netkit::Websocket ws(wsHostname, wsPort, config, endpoint);

  cexkit::binance::DataHub datahub(&rest, &ws);
  cexkit::binance::Broker broker(&rest, &ws);

  std::cout << "datahub setup done!\n";
  // std::this_thread::sleep_for(std::chrono::seconds(2));

  // datahub.subscribe("solusdt@depth@100ms");
  // datahub.registerCallback("solusdt@depth@100ms", depthCB);
  // datahub.subscribe("ethusdt@depth@100ms");
  // datahub.registerCallback("ethusdt@depth@100ms", depthCB);

  // std::this_thread::sleep_for(std::chrono::seconds(10));
  // datahub.unsubscribe("solusdt@depth@100ms");
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // datahub.subscribe("btcusdt@depth@100ms");
  // datahub.registerCallback("btcusdt@depth@100ms", depthCB);
  datahub.subscribeOrderBook("solusdt");
  datahub.join();
}