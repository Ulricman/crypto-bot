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

// Initialize OpenSSL library.
void initOpenssl() {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

// Cleanup OpenSSL resources.
void cleanupOpenssl() { EVP_cleanup(); }

SSL_CTX* createSSLContext(const char* caPath) {
  const SSL_METHOD* method = TLS_client_method();
  SSL_CTX* ctx = SSL_CTX_new(method);

  if (!ctx) {
    ERR_print_errors_fp(stderr);
    throw std::runtime_error("Failed creating SSL context");
  }

  // Verify CA.
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  SSL_CTX_load_verify_locations(ctx, caPath, nullptr);

  return ctx;
}

}  // namespace netkit