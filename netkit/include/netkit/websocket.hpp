#ifndef __NETKIT_WEBSOCKET_HPP__
#define __NETKIT_WEBSOCKET_HPP__

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <new>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "netkit/proxy.hpp"
#include "netkit/utils.hpp"

namespace netkit {

enum Opcode {
  CONTINUATION_FRAME = 0x0,
  TEXT_FRAME = 0x1,
  BINARY_FRAME = 0x2,
  CLOSE_FRAME = 0x8,
  PING_FRAME = 0x9,
  PONG_FRAME = 0xA,
};

struct Frame {
  bool fin, masked;
  Opcode opcode;
  std::string payload;

  Frame() = delete;
  Frame(bool _fin_, bool _masked_, Opcode _opcode_, const char* _payload_,
        uint64_t _payloadLen_)
      : fin(_fin_),
        masked(_masked_),
        opcode(_opcode_),
        payload(_payload_, _payloadLen_) {}
  Frame(bool _fin_, bool _masked_, Opcode _opcode_, std::string&& _payload_)
      : fin(_fin_),
        masked(_masked_),
        opcode(_opcode_),
        payload(std::move(_payload_)) {}

  ~Frame() {}
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;

  Frame(Frame&& frame)
      : fin(frame.fin),
        masked(frame.masked),
        opcode(frame.opcode),
        payload(std::move(frame.payload)) {}

  Frame& operator=(Frame&& frame) {
    fin = frame.fin;
    masked = frame.masked;
    opcode = frame.opcode;
    payload = std::move(frame.payload);
    return *this;
  }
};

class Websocket {
  const std::string hostname_;
  const unsigned int port_;
  const std::string endpoint_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  int sockFd_;
  SSL_CTX* ctx_;
  SSL* ssl_;
  const std::string apiKey_;
  const std::string apiSecret_;

  std::set<std::string> streams_;
  std::map<std::string, std::function<void(Frame)>> callbacks_;

  std::thread* streamLoop_;

 private:
  Frame parseWebsocketFrame(const char* buffer, size_t len);
  void sendWebsocketFrame(Frame frame);

  /**
   * * Establish a websocket connection to a specific endpoint, which
   * * needs to be called before any subscription or other requests.
   */
  bool connect(const std::string& endpoint);

  void streamLoop();

  // TODO: use move assignment operator.
  void pong(std::string);

 public:
  Websocket(const std::string& hostname, const unsigned int port,
            const std::string& caPath, const std::string& apiKey,
            const std::string& apiSecret, const std::string& endpoint_,
            const std::string& proxyHostname = "",
            const unsigned int proxyPort = 0);
  ~Websocket();

  void join();
  void subscribe(const std::string& stream);
  void subscribe(const std::vector<std::string>& streams);
  void unsubscribe(const std::string& stream);
  void unsubscribe(const std::vector<std::string>& streams);
  void listSubscriptions();

  // * returns the number of streams managed by this websocket.
  int numStreams() const;

  // * Register a callback for a specific stream.
  void registerCallback(const std::string& stream,
                        const std::function<void(Frame)>&);
};  // Websocket

}  // namespace netkit

#endif