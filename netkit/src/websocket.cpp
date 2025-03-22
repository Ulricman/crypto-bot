#include "netkit/websocket.hpp"

namespace netkit {

Websocket::Websocket(const std::string& hostname, const unsigned int port,
                     const std::string& caPath, const std::string& apiKey,
                     const std::string& apiSecret,
                     const std::string& proxyHostname,
                     const unsigned int proxyPort)
    : hostname_(hostname),
      port_(port),
      apiKey_(apiKey),
      apiSecret_(apiSecret),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort) {
  // Create socket or proxy tunnel if proxy is given.
  if (!proxyHostname.empty()) {
    sockFd_ = proxyTunnel(proxyHostname_, proxyPort_, hostname_, port_);
  } else {
    sockFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ < 0) {
      throw std::runtime_error(std::string("Failed creating socket: ") +
                               std::strerror(errno));
    }
  }

  // Initialize OpenSSL and define SSL context.
  initOpenssl();
  ctx_ = createSSLContext(caPath.data());

  // Create SSL object and bind to socket.
  ssl_ = SSL_new(ctx_);
  SSL_set_fd(ssl_, sockFd_);
  SSL_set_tlsext_host_name(ssl_, hostname_.data());

  // Perform SSL handshake.
  if (SSL_connect(ssl_) != 1) {
    ERR_print_errors_fp(stderr);
    SSL_free(ssl_);
    close(sockFd_);
    throw std::runtime_error(std::string("SSL handshake failed"));
  }

  std::cout << "SSL connected using cipher: " << SSL_get_cipher(ssl_)
            << std::endl;
}

Websocket::~Websocket() {
  // Cleanup OpenSSL resources.
  SSL_shutdown(ssl_);
  SSL_free(ssl_);
  close(sockFd_);
  SSL_CTX_free(ctx_);
  cleanupOpenssl();
}

std::string Websocket::parseWebsocketFrame(const char* buffer, size_t len) {
  const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
  bool fin = (data[0] & 0x80) != 0;
  Opcode opcode = static_cast<Opcode>(data[0] & 0x0F);
  bool masked = (data[1] & 0x80);
  size_t payloadLen = data[1] & 0x7F;

  size_t offset = 2;
  if (payloadLen == 126) {
    payloadLen = (data[2] << 8) | data[3];
    offset += 2;
  } else if (payloadLen == 127) {
    return "";
  }

  // Parse masks if has.
  char mask[4];
  if (masked) {
    memcpy(mask, &data[offset], 4);
    offset += 4;
  }

  // Parse data.
  std::string payload;
  for (size_t i = 0; i < payloadLen; ++i) {
    char byte = data[offset + i];
    if (masked) {
      byte ^= mask[i % 4];
    }
    payload.push_back(byte);
  }

  return payload;
}

void Websocket::subscribe(const std::string& stream) {
  // Perform Websocket handshake.
  std::string wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

  std::string upgradeRequest = "GET /ws/" + stream +
                               " HTTP/1.1\r\n"
                               "Host: " +
                               hostname_ +
                               "\r\n"
                               "Upgrade: websocket\r\n"
                               "Connection: Upgrade\r\n"
                               "Sec-WebSocket-Key: " +
                               wsKey +
                               "\r\n"
                               "Sec-WebSocket-Version: 13\r\n\r\n";
  if (SSL_write(ssl_, upgradeRequest.data(), upgradeRequest.size()) <= 0) {
    throw std::runtime_error("Failed sending upgrade request");
  }

  char buffer[4096];
  int bytes = SSL_read(ssl_, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    throw std::runtime_error("Failed receving websocket handshake response");
  }
  buffer[bytes] = '\0';

  std::string response(buffer, bytes);
  if (response.find("HTTP/1.1 101 Switching Protocols") == std::string::npos) {
    throw std::runtime_error("Websocket handshake failed");
  }

  // Keep receving data.
  while (true) {
    bytes = SSL_read(ssl_, buffer, sizeof(buffer));
    if (bytes <= 0) {
      throw std::runtime_error("Disconnected");
    }

    // Parse websocket frame.
    std::string payload = parseWebsocketFrame(buffer, bytes);
    if (!payload.empty()) {
      std::cout << payload << std::endl;
    }
  }
}

void Websocket::subscribe(const std::vector<std::string>& streams) {
  // Perform Websocket handshake.
  std::string wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

  std::ostringstream oss;
  oss << "GET /stream?streams=";
  for (auto it = streams.cbegin(); it != streams.cend(); ++it) {
    if (it != streams.cbegin()) {
      oss << "/";
    }
    oss << *it;
  }
  oss << " HTTP/1.1\r\n";
  oss << "Host: " << hostname_ << "\r\n";
  oss << "Upgrade: websocket\r\n";
  oss << "Connection: Upgrade\r\n";
  oss << "Sec-WebSocket-Key: " << wsKey << "\r\n";
  oss << "Sec-WebSocket-Version: 13\r\n\r\n";

  std::string upgradeRequest = oss.str();
  if (SSL_write(ssl_, upgradeRequest.data(), upgradeRequest.size()) <= 0) {
    throw std::runtime_error("Failed sending upgrade request");
  }

  char buffer[4096];
  int bytes = SSL_read(ssl_, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    throw std::runtime_error("Failed receving websocket handshake response");
  }
  buffer[bytes] = '\0';

  std::string response(buffer, bytes);
  if (response.find("HTTP/1.1 101 Switching Protocols") == std::string::npos) {
    throw std::runtime_error("Websocket handshake failed");
  }

  // Keep receving data.
  while (true) {
    bytes = SSL_read(ssl_, buffer, sizeof(buffer));
    if (bytes <= 0) {
      throw std::runtime_error("Disconnected");
    }

    // Parse websocket frame.
    std::string payload = parseWebsocketFrame(buffer, bytes);
    if (!payload.empty()) {
      std::cout << payload << std::endl;
    }
  }
}

}  // namespace netkit