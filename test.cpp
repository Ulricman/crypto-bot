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

// Resolve hostname.
std::string resolveHostname(const std::string& hostname) {
  struct hostent* host = gethostbyname(hostname.data());
  if (!host) {
    throw std::runtime_error("Failed resolve hostname");
  }
  return inet_ntoa(*((in_addr*)(host->h_addr_list[0])));
}

int main() {
  init_openssl();
  SSL_CTX* ctx = create_ssl_context();

  const std::string hostname = "api.binance.com";
  const int port = 443;
  const std::string proxyHostname = "127.0.0.1";
  const int proxyPort = 20112;

  // Resolve IP.
  std::string ip = resolveHostname(proxyHostname);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(proxyPort);
  inet_pton(AF_INET, ip.data(), &server_addr.sin_addr);

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    throw std::runtime_error(std::string("Failed creating socket: ") +
                             std::strerror(errno));
  }

  if (connect(sock_fd, reinterpret_cast<struct sockaddr*>(&server_addr),
              sizeof(server_addr)) < 0) {
    close(sock_fd);
    throw std::runtime_error(std::string("Failed connecting to proxy: ") +
                             std::strerror(errno));
  }

  std::string connect_cmd = "CONNECT " + hostname + ":" + std::to_string(port) +
                            " HTTP/1.1\r\n"
                            "Host: " +
                            hostname + ":" + std::to_string(port) +
                            "\r\n"
                            "Proxy-Connection: Keep-Alive\r\n\r\n";

  if (send(sock_fd, connect_cmd.data(), connect_cmd.size(), 0) < 0) {
    close(sock_fd);
    std::runtime_error(
        std::string("Failed establish connection on the proxy: ") +
        std::strerror(errno));
  }

  char buffer[1024];
  ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
  if (bytes <= 0) {
    close(sock_fd);
    close(1);
    std::runtime_error(std::string("Proxy response error: ") +
                       std::strerror(errno));
  }

  std::string response(buffer, bytes);
  if (response.find("200 Connection established") == std::string::npos) {
    close(sock_fd);
    std::runtime_error(std::string("Proxy error: ") + std::strerror(errno));
  }

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
      "GET /api/v3/time HTTP/1.1\r\n"
      "Host: " +
      hostname +
      "\r\n"
      "User-Agent: OpenSSL/1.1.1\r\n"
      "Connection: close\r\n\r\n";

  SSL_write(ssl, request.data(), request.size());

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