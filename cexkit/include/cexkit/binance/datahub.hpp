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

  int maxNumStreams_ = 10;

  netkit::Rest rest_;
  netkit::Websocket ws_;

  std::set<std::string> streams_;  // Subscribed streams.
  std::map<std::string, OrderBook> orderbooks_;

 public:
  DataHub(const std::string &hostname, const unsigned int port,
          const std::string &caPath, const std::string &apiKey,
          const std::string &apiSecret, const std::string &endpoint,
          const std::string &proxyHostname = "",
          const unsigned int proxyPort = 0);

  // Subscribe a stream (streams) through websocket.
  void subscribe(const std::string &stream);
  void subscribe(const std::vector<std::string> &streams);

  // Unsubscribe a stream (streams) on websocket.
  void unsubscribe(const std::string &stream);
  void unsubscribe(const std::vector<std::string> &streams);

  void listSubscriptopns();

  // Set the maximum number of streams one websocket can listen.
  void setMaxNumStreams(int val) { maxNumStreams_ = val; }
};  // DataHub

}  // namespace binance

}  // namespace cexkit

#endif