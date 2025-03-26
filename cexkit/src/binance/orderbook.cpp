#include "cexkit/binance/orderbook.hpp"

namespace cexkit {
namespace binance {

OrderBook::OrderBook() : lastUpdateId_(0) {}

void OrderBook::update(std::map<price_t, qty_t>&& bidOrders,
                       std::map<price_t, qty_t>&& askOrders,
                       uint64_t lastUpdateId) {
  // Discard snapshot that happens before the current orderbook.
  if (lastUpdateId < lastUpdateId_.load(std::memory_order_acquire)) {
    return;
  }

  bidOrders_ = bidOrders;
  askOrders_ = askOrders;
  lastUpdateId_.store(lastUpdateId, std::memory_order_release);
}

void OrderBook::update(price_t price, qty_t qty, uint64_t firstUpdateId,
                       uint64_t finalUpdateId, bool isBid) {
  // Discard event that happens before the current orderbook.
  if (finalUpdateId < lastUpdateId_.load(std::memory_order_acquire)) {
    return;
  }

  if (isBid) {
    // Update Bid orders.
    if (qty == 0) {
      if (bidOrders_.contains(price)) {
        bidOrders_.erase(price);
      }
    } else {
      bidOrders_[price] = qty;
    }
  } else {
    // Update Ask orders.
    if (qty == 0) {
      if (askOrders_.contains(price)) {
        askOrders_.erase(price);
      }
    } else {
      askOrders_[price] = qty;
    }
  }

  // Update `lastUpdateId`.
  lastUpdateId_.store(finalUpdateId, std::memory_order_release);
}

}  // namespace binance
}  // namespace cexkit