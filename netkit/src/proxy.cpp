#include "netkit/proxy.hpp"

namespace netkit {

int proxyTunnel(const std::string& proxyHostname, unsigned int proxyPort,
                const std::string& hostname, unsigned int port) {
  // Resolve IP.
  std::string ip = resolveHostname(proxyHostname);

  struct sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(proxyPort);
  inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr);

  // Create socket.
  int sockFd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockFd < 0) {
    throw std::runtime_error(
        std::string("Failed creating socket when establishing proxy tunnel: ") +
        std::strerror(errno));
  }

  if (connect(sockFd, reinterpret_cast<struct sockaddr*>(&serverAddress),
              sizeof(serverAddress)) < 0) {
    close(sockFd);
    throw std::runtime_error(std::string("Failed connecting to proxy: ") +
                             std::strerror(errno));
  }

  // Define the connection request.
  std::string connectCmd = "CONNECT " + hostname + ":" + std::to_string(port) +
                           " HTTP/1.1\r\n"
                           "Host: " +
                           hostname + ":" + std::to_string(port) +
                           "\r\n"
                           "Proxy-Connection: Keep-Alive\r\n\r\n";

  // Send the connection request.
  if (send(sockFd, connectCmd.data(), connectCmd.size(), 0) < 0) {
    close(sockFd);
    throw std::runtime_error(
        std::string("Failed sending tunnel connection request to proxy: ") +
        std::strerror(errno));
  }

  char buffer[1024];
  ssize_t bytes = recv(sockFd, buffer, sizeof(buffer), 0);
  if (bytes <= 0) {
    close(sockFd);
    throw std::runtime_error(std::string("Proxy response error: ") +
                             std::strerror(errno));
  }

  // Check whether the tunnel is established successfully.
  std::string response(buffer, bytes);
  if (response.find("200 Connection established") == std::string::npos) {
    close(sockFd);
    throw std::runtime_error("Failed establish tunnel");
  }

  return sockFd;
}

int proxyTunnel(const char* proxyHostname, unsigned int proxyPort,
                const char* hostname, unsigned int port) {
  return proxyTunnel(std::string(proxyHostname), proxyPort,
                     std::string(hostname), port);
}

}  // namespace netkit