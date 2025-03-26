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

void DataHub::registerCallback(const std::string &stream,
                               const std::function<void(netkit::Frame)> &cb) {
  ws_.registerCallback(stream, cb);
}

}  // namespace binance

}  // namespace cexkit