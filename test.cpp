#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

std::string resolveHostname(const std::string& hostname) {
  struct hostent* host = gethostbyname(hostname.data());
  if (!host) {
    throw std::runtime_error("Failed resolve hostname");
  }
  return inet_ntoa(*((in_addr*)(host->h_addr_list[0])));
}

int main() {
  const std::string hostname = "httpbin.org";
  const int port = 80;

  // Resolve IP.
  std::string ip = resolveHostname(hostname);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, ip.data(), &server_addr.sin_addr);

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    throw std::runtime_error(std::string("Failed creating socket: ") +
                             std::strerror(errno));
    return 1;
  }

  if (connect(sock_fd, reinterpret_cast<struct sockaddr*>(&server_addr),
              sizeof(server_addr)) < 0) {
    throw std::runtime_error(std::string("Failed connecting to server: ") +
                             std::strerror(errno));
    close(sock_fd);
    return 1;
  }

  std::string request =
      "GET /get HTTP/1.1\r\n"
      "Host: " +
      hostname +
      "\r\n"
      "User-Agent: C++Socket/1.0\r\n"
      "Connection: close\r\n\r\n";

  if (send(sock_fd, request.data(), request.size(), 0) < 0) {
    throw std::runtime_error(std::string("Failed sending request: ") +
                             std::strerror(errno));
    close(sock_fd);
    return 1;
  }

  char buffer[1024];
  std::string response;
  ssize_t bytes;

  while ((bytes = recv(sock_fd, buffer, sizeof(buffer), 0)) > 0) {
    response.append(buffer, bytes);
  }
  close(sock_fd);
  std::cout << response << std::endl;
}