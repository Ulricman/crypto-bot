#ifndef __NETKIT_WEBSOCKET_HPP__
#define __NETKIT_WEBSOCKET_HPP__

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

#include "netkit/proxy.hpp"
#include "netkit/utils.hpp"

namespace netkit {

enum Opcode {
  TEXT_FRAME = 0x1,
  BINARY_FRAME = 0x2,
  CLOSE_FRAME = 0x8,
  PING_FRAME = 0x9,
  PONG_FRAME = 0xA,
};

class Websocket {
  const std::string hostname_;
  const unsigned int port_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  int sockFd_;
  SSL_CTX* ctx_;
  SSL* ssl_;
  const std::string apiKey_;
  const std::string apiSecret_;

 private:
  std::string parseWebsocketFrame(const char* buffer, size_t len);

 public:
  Websocket(const std::string& hostname, const unsigned int port,
            const std::string& caPath, const std::string& apiKey,
            const std::string& apiSecret, const std::string& proxyHostname = "",
            const unsigned int proxyPort = 0);
  ~Websocket();

  void subscribe(const std::string& stream);
  void subscribe(const std::vector<std::string>& streams);
};  // Websocket

}  // namespace netkit

#endif