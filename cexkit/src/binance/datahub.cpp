#include "cexkit/binance/datahub.hpp"

namespace cexkit {

namespace binance {

DataHub::DataHub(const std::string &restHostname, const unsigned int restPort,
                 const std::string &wsHostname, const unsigned int wsPort,
                 const std::string &caPath, const std::string &apiKey,
                 const std::string &apiSecret, const std::string &endpoint,
                 const std::string &proxyHostname, const unsigned int proxyPort)
    : restHostname_(restHostname),
      restPort_(restPort),
      wsHostname_(wsHostname),
      wsPort_(wsPort),
      endpoint_(endpoint),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort),
      apiKey_(apiKey),
      apiSecret_(apiSecret),
      rest_(restHostname, restPort, caPath, apiKey, apiSecret, proxyHostname,
            proxyPort),
      ws_(wsHostname, wsPort, caPath, apiKey, apiSecret, endpoint,
          proxyHostname, proxyPort) {
  if (endpoint_.empty()) {
    throw std::runtime_error(
        "Must pass a valid endpoint to establish websocket connection");
  }
}

DataHub::~DataHub() {
  for (const auto &orderbook : orderbooks_) {
    delete orderbook.second;
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

void DataHub::subscribeOrderBook(const std::string &symbol) {
  if (orderbooks_.contains(symbol)) {
    throw std::runtime_error(std::string("Local OrderBook of ") + symbol +
                             std::string(" has already been maintained"));
  }
  orderbooks_[symbol] = new OrderBook(eventBufferSize_);
  std::string stream = symbol + "@depth@100ms";

  // Subscribe websocket stream from exchange.
  subscribe(stream);
  registerCallback(stream,
                   [this](netkit::Frame frame) { updateOBByEvent(frame); });

  // Wait until orderbook has buffered some events, so that we can check
  // the validity of snapshot.
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Fetch snapshot from exchange rest API.
  while (true) {
    std::string snapshot = rest_.sendPublicRequest(
        "/api/v3/depth", "GET", {{"symbol", upper(symbol)}, {"limit", "10"}});
    if (orderbooks_[symbol]->initDepth(std::move(snapshot))) {
      break;
    }
  }
}

void DataHub::unsubscribeOrderBook(const std::string &symbol) {
  std::string stream = symbol + "@depth@100ms";
  unsubscribe(stream);
  delete orderbooks_[symbol];
}

void DataHub::listSubscriptopns() { ws_.listSubscriptions(); }

void DataHub::updateOBByEvent(netkit::Frame frame) {
  nlohmann::json payload = nlohmann::json::parse(frame.payload);
  std::string stream = payload["stream"];
  std::string symbol = stream.substr(0, stream.find_first_of("@"));

  OrderBook *orderbook = orderbooks_[symbol];
  if (orderbook == nullptr) [[unlikely]] {
    std::cerr << "OrderBook for " << symbol << " does not exist\n";
  }
  orderbook->pushEvent(std::move(frame.payload));
}

void DataHub::registerCallback(const std::string &stream,
                               const std::function<void(netkit::Frame)> &cb) {
  ws_.registerCallback(stream, cb);
}

}  // namespace binance

}  // namespace cexkit