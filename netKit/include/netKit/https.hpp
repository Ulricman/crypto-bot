#ifndef __NETKIT_HTTPS_HPP__
#define __NETKIT_HTTPS_HPP__

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <sstream>
#include <string>
#include <unordered_map>

#include "netKit/proxy.hpp"

namespace netkit {

// * An agent perform HTTPS requests.
class Agent {
  const std::string hostname_;
  const unsigned int port_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  int sockFd_;
  SSL_CTX* ctx_;
  SSL* ssl_;

 public:
  Agent(const std::string& _hostname_, const unsigned int _port_,
        const char* caPath, const std::string& _proxyHostname_ = "",
        const unsigned int _proxyPort_ = 0);
  Agent(const Agent&) = delete;
  Agent& operator=(const Agent&) = delete;
  Agent(Agent&&) = delete;
  Agent& operator=(Agent&&) = delete;

  std::string request(
      const std::string& url,
      const std::unordered_map<std::string, std::string>& params);
  ~Agent();
};  // Agent

}  // namespace netkit

#endif