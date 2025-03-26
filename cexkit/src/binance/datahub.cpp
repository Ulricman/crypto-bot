#include "cexkit/binance/datahub.hpp"

namespace cexkit {

namespace binance {

DataHub::DataHub(const std::string &hostname, const unsigned int port,
                 const std::string &caPath, const std::string &apiKey,
                 const std::string &apiSecret, const std::string &endpoint,
                 const std::string &proxyHostname, const unsigned int proxyPort)
    : hostname_(hostname),
      port_(port),
      endpoint_(endpoint),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort),
      apiKey_(apiKey),
      apiSecret_(apiSecret),
      rest_(hostname, port, caPath, apiKey, apiSecret, proxyHostname,
            proxyPort),
      ws_(hostname, port, caPath, apiKey, apiSecret, endpoint, proxyHostname,
          proxyPort) {
  if (endpoint_.empty()) {
    throw std::runtime_error(
        "Must pass a valid endpoint to establish websocket connection");
  }
}

void DataHub::subscribe(const std::string &stream) {
  streams_.insert(stream);
  ws_.subscribe(stream);
}

void DataHub::subscribe(const std::vector<std::string> &streams) {
  for (const auto &stream : streams) {
    subscribe(stream);
  }
}

void DataHub::unsubscribe(const std::string &stream) {
  if (streams_.contains(stream)) {
    streams_.erase(stream);
    ws_.unsubscribe(stream);
  } else {
    std::cerr << "Stream " << stream << " was not subscribed before\n";
  }
}

void DataHub::unsubscribe(const std::vector<std::string> &streams) {
  for (const auto &stream : streams) {
    unsubscribe(stream);
  }
}

void DataHub::listSubscriptopns() { ws_.listSubscriptions(); }

void DataHub::updateOBByEvent(netkit::Frame frame) {
  nlohmann::json payload = nlohmann::json::parse(frame.payload);
  OrderBook &orderbook = orderbooks_[payload["stream"]];
  uint64_t firstUpdateId = payload["data"]["U"];
  uint64_t finalUpdateId = payload["data"]["u"];

  // Assume both price and qty have 8 decimal places.
  for (std::vector<std::string> bid : payload["data"]["b"]) {
    bid[0].erase(bid[0].size() - 9, 1);
    bid[1].erase(bid[1].size() - 9, 1);
    uint64_t price = static_cast<uint64_t>(std::stoull(bid[0]));
    uint64_t qty = static_cast<uint64_t>(std::stoull(bid[1]));
    std::cout << "Update: " << payload["stream"] << " | bid: " << price << " , "
              << qty << std::endl;

    orderbook.update(price, qty, firstUpdateId, finalUpdateId, true);
  }

  for (std::vector<std::string> ask : payload["data"]["a"]) {
    ask[0].erase(ask[0].size() - 9, 1);
    ask[1].erase(ask[1].size() - 9, 1);
    uint64_t price = static_cast<uint64_t>(std::stoull(ask[0]));
    uint64_t qty = static_cast<uint64_t>(std::stoull(ask[1]));
    std::cout << "Update: " << payload["stream"] << " | ask: " << price << " , "
              << qty << std::endl;

    orderbook.update(price, qty, firstUpdateId, finalUpdateId, false);
  }
}

void DataHub::registerCallback(const std::string &stream,
                               const std::function<void(netkit::Frame)> &cb) {
  ws_.registerCallback(stream, cb);
}

}  // namespace binance

}  // namespace cexkit