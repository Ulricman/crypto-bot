#ifndef __CEXKIT_BINANCE_ORDERBOOK_HPP__
#define __CEXKIT_BINANCE_ORDERBOOK_HPP__

#include <atomic>
#include <map>

#include "cexkit/cexdef.hpp"

namespace cexkit {

namespace binance {
class OrderBook {
 private:
  std::atomic<uint64_t> lastUpdateId_;
  std::map<price_t, qty_t> bidOrders_, askOrders_;

 public:
  OrderBook();

  /**
   * * This function should be called to update the orderbook according
   * * to the snapshot received from Partial Book Depth Stream.
   */
  void update(std::map<price_t, qty_t>&& bidOrders,
              std::map<price_t, qty_t>&& askOrders, uint64_t lastUpdateId);

  /**
   * * This function should be called to update the orderbook according
   * * to the event received from Diff.Depth Stream.
   */
  void update(price_t price, qty_t qty, uint64_t firstUpdateId,
              uint64_t finalUpdateId, bool isBid);
};  // OrderBook

}  // namespace binance

}  // namespace cexkit

#endif