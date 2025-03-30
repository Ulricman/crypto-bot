#ifndef __CEXKIT_BINANCE_ORDERBOOK_HPP__
#define __CEXKIT_BINANCE_ORDERBOOK_HPP__

#include <atomic>
#include <iostream>
#include <map>
#include <string>

#include "cexkit/cexdef.hpp"
#include "cexkit/ringbuffer.hpp"
#include "nlohmann/json.hpp"

namespace cexkit {

namespace binance {
class OrderBook {
 private:
  std::string symbol_;
  std::atomic<uint64_t> lastUpdateId_;
  std::map<price_t, qty_t> bidOrders_, askOrders_;
  RingBuffer<std::string> events_;

 private:
  //* This function is called to update orderbook from events in the buffer.
  void update();

 public:
  OrderBook(const std::string& symbol, uint64_t eventBufferSize);

  bool initDepth(std::string&& payload);

  void pushEvent(const std::string& event);
  void pushEvent(std::string&& event);
};  // OrderBook

}  // namespace binance

}  // namespace cexkit

#endif