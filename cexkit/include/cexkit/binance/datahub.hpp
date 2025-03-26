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

 private:
  /**
   * * Callback updating local orderbook through event received from Diff.Depth
   * * Stream. This callback assumes the OrderBook object has been instantiated.
   * * And the caller needs to make sure the payload of passed frame contains
   * * the "stream" field.
   */
  void updateOBByEvent(netkit::Frame frame);

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

  void localOrderBook(const std::string &symbol) {
    if (orderbooks_.contains(symbol)) {
      throw std::runtime_error(std::string("Local OrderBook of ") + symbol +
                               std::string(" has already been maintained"));
    }
    orderbooks_[symbol];
    std::string stream = symbol + "@depth@100ms";
    subscribe(stream);
    registerCallback(stream,
                     [this](netkit::Frame frame) { updateOBByEvent(frame); });
  }

  // Set the maximum number of streams one websocket can listen.
  void setMaxNumStreams(int val) { maxNumStreams_ = val; }

  void registerCallback(const std::string &stream,
                        const std::function<void(netkit::Frame)> &);
};  // DataHub

}  // namespace binance

}  // namespace cexkit

#endif