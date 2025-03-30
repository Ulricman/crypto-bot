#include "cexkit/binance/orderbook.hpp"

namespace cexkit {
namespace binance {

OrderBook::OrderBook(const std::string& symbol, uint64_t eventBufferSize)
    : symbol_(symbol), lastUpdateId_(0), events_(eventBufferSize) {}

bool OrderBook::initDepth(std::string&& payload) {
  nlohmann::json snapshot = nlohmann::json::parse(payload);

  uint64_t lastUpdateId = static_cast<uint64_t>(snapshot["lastUpdateId"]);

  // Check the first buffered event.
  std::string* event = nullptr;
  events_.frontBlock(&event);
  nlohmann::json diffDepth = nlohmann::json::parse(*event);
  uint64_t firstUpdateId = static_cast<uint64_t>(diffDepth["data"]["U"]);

  // If the snapshot is outdated, do nothing.
  if (lastUpdateId < firstUpdateId) {
    return false;
  }

  // Update bid orders.
  for (std::vector<std::string> bid : snapshot["bids"]) {
    bid[0].erase(bid[0].size() - 9, 1);
    bid[1].erase(bid[1].size() - 9, 1);
    price_t price = static_cast<price_t>(std::stoull(bid[0]));
    qty_t qty = static_cast<qty_t>(std::stoull(bid[1]));

    if (qty > 0) {
      bidOrders_[price] = qty;
    } else {
      if (bidOrders_.contains(price)) {
        bidOrders_.erase(price);
      }
    }
  }

  // Update ask orders.
  for (std::vector<std::string> ask : snapshot["asks"]) {
    ask[0].erase(ask[0].size() - 9, 1);
    ask[1].erase(ask[1].size() - 9, 1);
    price_t price = static_cast<price_t>(std::stoull(ask[0]));
    qty_t qty = static_cast<qty_t>(std::stoull(ask[1]));

    if (qty > 0) {
      askOrders_[price] = qty;
    } else {
      if (askOrders_.contains(price)) {
        askOrders_.erase(price);
      }
    }
  }

  lastUpdateId_.store(lastUpdateId, std::memory_order_release);
  std::cout << "snapshot lastUpdateId = " << lastUpdateId << std::endl;

  return true;
}

void OrderBook::update() {
  if (lastUpdateId_.load(std::memory_order_acquire) == 0) {
    return;
  }

  std::string* event = nullptr;
  while (events_.frontNonBlock(&event)) {
    nlohmann::json diffDepth = nlohmann::json::parse(*event);
    uint64_t firstUpdateId = static_cast<uint64_t>(diffDepth["data"]["U"]);
    uint64_t finalUpdateId = static_cast<uint64_t>(diffDepth["data"]["u"]);
    std::cout << "firstUpdateId = " << firstUpdateId
              << " | finalUpdateId = " << finalUpdateId
              << " | lastUpdateIdx_ = " << lastUpdateId_ << std::endl;

    if (finalUpdateId <= lastUpdateId_.load(std::memory_order_acquire)) {
      events_.popNonBlock(true);
      continue;
    }
    if (firstUpdateId > lastUpdateId_.load(std::memory_order_acquire) + 1) {
      // TODO: to be dealed with.
      std::cerr << "Depth corrupted\n";
    }

    // Update bid orders.
    for (std::vector<std::string> bid : diffDepth["data"]["b"]) {
      bid[0].erase(bid[0].size() - 9, 1);
      bid[1].erase(bid[1].size() - 9, 1);
      price_t price = static_cast<price_t>(std::stoull(bid[0]));
      qty_t qty = static_cast<qty_t>(std::stoull(bid[1]));

      if (qty > 0) {
        bidOrders_[price] = qty;
      } else {
        if (bidOrders_.contains(price)) {
          bidOrders_.erase(price);
        }
      }
    }

    // Update ask orders.
    for (std::vector<std::string> ask : diffDepth["data"]["a"]) {
      ask[0].erase(ask[0].size() - 9, 1);
      ask[1].erase(ask[1].size() - 9, 1);
      price_t price = static_cast<price_t>(std::stoull(ask[0]));
      qty_t qty = static_cast<qty_t>(std::stoull(ask[1]));

      if (qty > 0) {
        askOrders_[price] = qty;
      } else {
        if (askOrders_.contains(price)) {
          askOrders_.erase(price);
        }
      }
    }

    lastUpdateId_.store(finalUpdateId, std::memory_order_release);
    events_.popBlock(true);

    // Dispaly depth.
    std::cout << "update | " << symbol_ << std::endl;
    std::cout << "bidOrders_.size() = " << bidOrders_.size() << std::endl;
    std::cout << "askOrders_.size() = " << askOrders_.size() << std::endl;
    {
      auto it = askOrders_.cbegin();
      for (int i = 0; i < 5; ++i, ++it) {
        std::cout << "A[" << it->first << ", " << it->second << "]\n";
      }
    }
    std::cout << "---------------------------\n";
    {
      auto it = bidOrders_.crbegin();
      for (int i = 0; i < 5; ++i, ++it) {
        std::cout << "B[" << it->first << ", " << it->second << "]\n";
      }
    }
  }
}

void OrderBook::pushEvent(const std::string& event) {
  events_.pushBlock(event);
  std::cout << "bufferSize = " << events_.size() << std::endl;
  update();
}

void OrderBook::pushEvent(std::string&& event) {
  events_.pushBlock(std::move(event));
  std::cout << "bufferSize = " << events_.size() << std::endl;
  update();
}

}  // namespace binance

}  // namespace cexkit