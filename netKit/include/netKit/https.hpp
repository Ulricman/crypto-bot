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
  const std::string apiKey_;
  const std::string apiSecret_;

 private:
  std::string joinParams(
      const std::unordered_map<std::string, std::string>& params);
  std::string executeRequest(
      const std::string& url, const std::string& httpMethod,
      const std::unordered_map<std::string, std::string>& headers = {});

 public:
  Agent(const std::string& hostname, const unsigned int port,
        const char* caPath, const std::string& apiKey,
        const std::string& apiSecret, const std::string& proxyHostname = "",
        const unsigned int proxyPort = 0);
  Agent(const Agent&) = delete;
  Agent& operator=(const Agent&) = delete;
  Agent(Agent&&) = delete;
  Agent& operator=(Agent&&) = delete;
  ~Agent();

  std::string sendPublicRequest(
      std::string url, const std::string& httpMethod,
      const std::unordered_map<std::string, std::string>& params = {});
  std::string sendSignedRequest(
      std::string url, const std::string& httpMethod,
      const std::unordered_map<std::string, std::string>& params = {});

};  // Agent

}  // namespace netkit

#endif