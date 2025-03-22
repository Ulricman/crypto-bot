#include "netKit/utils.hpp"

namespace netkit {

std::string resolveHostname(const char* hostname) {
  struct addrinfo hints{}, *res;
  hints.ai_family = AF_INET;  // IPv4 only.
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(hostname, nullptr, &hints, &res)) {
    throw std::runtime_error("Failed resolving hostname");
  }

  if (res == nullptr) {
    throw std::runtime_error("No usable IP address found");
  }
  struct sockaddr_in* ipv4 =
      reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(ipv4->sin_addr), ip, sizeof(ip));
  freeaddrinfo(res);

  return ip;
}

std::string resolveHostname(const std::string& hostname) {
  return resolveHostname(hostname.data());
}

}  // namespace netkit