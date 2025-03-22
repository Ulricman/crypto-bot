#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "netKit/proxy.hpp"
#include "netKit/utils.hpp"

// Initialize OpenSSL library.
void init_openssl() {
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();
}

// Clean up OpenSSL resources.
void cleanup_openssl() { EVP_cleanup(); }

SSL_CTX* create_ssl_context() {
  const SSL_METHOD* method = TLS_client_method();
  SSL_CTX* ctx = SSL_CTX_new(method);
  if (!ctx) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  // Verify CA.
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
  SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs/ca-certificates.crt",
                                nullptr);
  return ctx;
}

int main() {
  init_openssl();
  SSL_CTX* ctx = create_ssl_context();

  const std::string hostname = "api.binance.com";
  const int port = 443;
  const std::string proxyHostname = "127.0.0.1";
  const int proxyPort = 20112;

  int sock_fd = netkit::proxyTunnel(proxyHostname, proxyPort, hostname, port);

  // Create SSL object and bind socket.
  SSL* ssl = SSL_new(ctx);
  SSL_set_fd(ssl, sock_fd);
  SSL_set_tlsext_host_name(ssl, hostname.data());

  // Perform SSL handshake.
  if (SSL_connect(ssl) != 1) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl);
    close(sock_fd);
    throw std::runtime_error(std::string("Failed established SSL handshake"));
  }

  std::cout << "SSL connected using cipher: " << SSL_get_cipher(ssl)
            << std::endl;

  std::string request =
      "GET /api/v3/depth?symbol=BTCUSDT HTTP/1.1\r\n"
      "Host: " +
      hostname +
      "\r\n"
      "User-Agent: OpenSSL/1.1.1\r\n"
      "Connection: close\r\n\r\n";

  SSL_write(ssl, request.data(), request.size());

  std::string response;
  ssize_t bytes;
  char buffer[1024];
  while ((bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
    response.append(buffer, bytes);
  }

  // Cleanup resources.
  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(sock_fd);
  SSL_CTX_free(ctx);
  cleanup_openssl();

  std::cout << response << std::endl;
}