#ifndef __NETKIT_REST_HPP__
#define __NETKIT_REST_HPP__

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <sstream>
#include <string>
#include <unordered_map>

#include "netkit/proxy.hpp"

namespace netkit {

// * An agent perform HTTPS requests.
class Rest {
  const std::string hostname_;
  const unsigned int port_;
  const std::string proxyHostname_;
  const unsigned int proxyPort_;
  int sockFd_;
  SSL_CTX *ctx_;
  SSL *ssl_;
  const std::string apiKey_;
  const std::string apiSecret_;

 private:
  std::string joinParams(
      const std::unordered_map<std::string, std::string> &params);
  std::string executeRequest(
      const std::string &url, const std::string &httpMethod,
      const std::unordered_map<std::string, std::string> &headers = {});

 public:
  Rest(const std::string &hostname, const unsigned int port,
       const std::string &caPath, const std::string &apiKey,
       const std::string &apiSecret, const std::string &proxyHostname = "",
       const unsigned int proxyPort = 0);
  Rest(const Rest &) = delete;
  Rest &operator=(const Rest &) = delete;
  Rest(Rest &&) = delete;
  Rest &operator=(Rest &&) = delete;
  ~Rest();

  std::string sendPublicRequest(
      std::string url, const std::string &httpMethod,
      const std::unordered_map<std::string, std::string> &params = {});
  std::string sendSignedRequest(
      std::string url, const std::string &httpMethod,
      const std::unordered_map<std::string, std::string> &params = {});

};  // Rest

}  // namespace netkit

#endif