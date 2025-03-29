#ifndef __CEXKIT_BINANCE_DATAHUB_HPP__
#define __CEXKIT_BINANCE_DATAHUB_HPP__

#include <iostream>
#include <map>
#include <set>
#include <string>

#include "cexkit/binance/orderbook.hpp"
#include "cexkit/utils.hpp"
#include "netkit/rest.hpp"
#include "netkit/websocket.hpp"

namespace cexkit {

namespace binance {

class DataHub {
 private:
  const std::string restHostname_;
  const unsigned int restPort_;
  const std::string wsHostname_;
  const unsigned int wsPort_;
  const std::string endpoint_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  const std::string apiKey_;
  const std::string apiSecret_;

  int maxNumStreams_ = 10;
  uint64_t eventBufferSize_ = 100;

  netkit::Rest rest_;
  netkit::Websocket ws_;

  std::set<std::string> streams_;  // Subscribed streams.
  std::map<std::string, OrderBook *> orderbooks_;

 private:
  /**
   * * Callback updating local orderbook through event received from Diff.Depth
   * * Stream. This callback assumes the OrderBook object has been instantiated.
   * * And the caller needs to make sure the payload of passed frame contains
   * * the "stream" field.
   */
  void updateOBByEvent(netkit::Frame frame);

 public:
  DataHub(const std::string &restHostname, const unsigned int restPort,
          const std::string &wsHostname, const unsigned int wsPort,
          const std::string &caPath, const std::string &apiKey,
          const std::string &apiSecret, const std::string &endpoint,
          const std::string &proxyHostname = "",
          const unsigned int proxyPort = 0);
  ~DataHub();

  void join();

  // Subscribe a stream (streams) through websocket.
  void subscribe(const std::string &stream);
  void subscribe(const std::vector<std::string> &streams);

  // Unsubscribe a stream (streams) on websocket.
  void unsubscribe(const std::string &stream);
  void unsubscribe(const std::vector<std::string> &streams);

  void listSubscriptopns();

  void subscribeOrderBook(const std::string &symbol);

  void unsubscribeOrderBook(const std::string &symbol);

  // Set the maximum number of streams one websocket can listen.
  void setMaxNumStreams(int val) { maxNumStreams_ = val; }

  void registerCallback(const std::string &stream,
                        const std::function<void(netkit::Frame)> &);
};  // DataHub

}  // namespace binance

}  // namespace cexkit

#endif