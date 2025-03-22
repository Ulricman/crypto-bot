#include "netkit/utils.hpp"

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

std::string getTimestamp() {
  auto msSinceEpoch = duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  return std::to_string(msSinceEpoch);
}

// Copied from
// https://github.dev/binance/binance-signature-examples/blob/master/cpp/spot.cpp.
std::string encryptWithHMAC(const char* key, const char* data) {
  unsigned char* result;
  static char res_hexstring[64];
  int result_len = 32;
  std::string signature;

  result = HMAC(
      EVP_sha256(), key, strlen((char*)key),
      const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(data)),
      strlen((char*)data), nullptr, nullptr);
  for (int i = 0; i < result_len; i++) {
    sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
  }

  for (int i = 0; i < 64; i++) {
    signature += res_hexstring[i];
  }

  return signature;
}

std::string getSignature(const std::string& key, const std::string& data) {
  return encryptWithHMAC(key.data(), data.data());
}

}  // namespace netkit