#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "netkit/netdef.hpp"
#include "netkit/websocket.hpp"

int main() {
  // Read configs.
  const char* configPath = "/home/jeffrey/crypto-bot/config.json";
  std::ifstream file(configPath);
  netkit::Config config;
  file >> config;
  file.close();

  const std::string hostname = "stream.binance.com";
  const int port = 9443;
  const std::string endpoint = "/stream";

  netkit::Websocket websocket(hostname, port, config, endpoint);

  const std::string stream = "btcusdt@depth@100ms";
  // std::vector<std::string> streams{"btcusdt@depth", "ethusdt@depth"};
  websocket.subscribe(stream);
}