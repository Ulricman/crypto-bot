#include "cexkit/binance/datahub.hpp"

namespace cexkit {

namespace binance {

DataHub::DataHub(const std::string &hostname, const unsigned int port,
                 const std::string &caPath, const std::string &apiKey,
                 const std::string &apiSecret, const std::string &proxyHostname,
                 const unsigned int proxyPort)
    : hostname_(hostname),
      port_(port),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort),
      apiKey_(apiKey),
      apiSecret_(apiSecret),
      rest_(hostname, port, caPath, apiKey, apiSecret, proxyHostname,
            proxyPort),
      ws_(hostname, port, caPath, apiKey, apiSecret, proxyHostname, proxyPort) {
}

void DataHub::subscribe(const std::string &stream) {
  streams.insert(stream);
  ws_.subscribe(stream);
}

void DataHub::unsubscribe(const std::string &stream) {
  if (streams.contains(stream)) {
    streams.erase(stream);
    ws_.unsubscribe(stream);
  } else {
    std::cerr << "Stream " << stream << " was not subscribed before\n";
  }
}

void DataHub::listSubscriptopns() { ws_.listSubscriptions(); }

}  // namespace binance

}  // namespace cexkit