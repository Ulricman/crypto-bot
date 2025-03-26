#ifndef __CEXKIT_BINANCE_DATAHUB_HPP__
#define __CEXKIT_BINANCE_DATAHUB_HPP__

#include <iostream>
#include <map>
#include <set>
#include <string>

#include "cexkit/binance/orderbook.hpp"
#include "netkit/rest.hpp"
#include "netkit/websocket.hpp"

namespace cexkit {

namespace binance {

class DataHub {
 private:
  const std::string hostname_;
  const unsigned int port_;
  const std::string endpoint_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  const std::string apiKey_;
  const std::string apiSecret_;

  netkit::Rest rest_;
  netkit::Websocket ws_;

  std::set<std::string> streams;  // Subscribed streams.
  std::map<std::string, OrderBook> orderbooks;

 public:
  DataHub(const std::string &hostname, const unsigned int port,
          const std::string &caPath, const std::string &apiKey,
          const std::string &apiSecret, const std::string &endpoint,
          const std::string &proxyHostname = "",
          const unsigned int proxyPort = 0);

  // Subscribe a stream through websocket.
  void subscribe(const std::string &stream);

  // Unsubscribe a stream on websocket.
  void unsubscribe(const std::string &stream);

  void listSubscriptopns();
};  // DataHub

}  // namespace binance

}  // namespace cexkit

#endif