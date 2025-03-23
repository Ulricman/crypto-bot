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

  // std::cout << "SSL connected using cipher: " << SSL_get_cipher(ssl_)
  //           << std::endl;
}

Websocket::~Websocket() {
  // Cleanup OpenSSL resources.
  SSL_shutdown(ssl_);
  SSL_free(ssl_);
  close(sockFd_);
  SSL_CTX_free(ctx_);
  cleanupOpenssl();
}

Frame Websocket::parseWebsocketFrame(const char* buffer, size_t len) {
  // For more detailed information about frame structure, refer to RFC6455:
  // https://datatracker.ietf.org/doc/html/rfc6455#autoid-21:~:text=).%0A%0A5.2.-,Base%20Framing%20Protocol,-This%20wire%20format
  const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
  Frame frame;
  frame.fin = (data[0] & 0x80) != 0;
  frame.opcode = static_cast<Opcode>(data[0] & 0x0F);
  frame.masked = data[1] & 0x80;
  size_t payloadLen = data[1] & 0x7F;

  if (frame.masked) {
    throw std::runtime_error("Error: received masked frame from server");
  }

  size_t offset = 2;
  if (payloadLen == 126) {
    payloadLen = (data[2] << 8) | data[3];
    offset += 2;
  } else if (payloadLen == 127) {
    payloadLen = 0;
    for (int i = 0; i < 8; ++i) {
      payloadLen |= static_cast<uint64_t>(data[2 + i]) << ((7 - i) * 8);
    }
    offset += 8;
  }

  // Extract payload. (Note that server MUST NOT mask data)
  // frame.payload = std::string(buffer, offset);
  for (int i = 0; i < payloadLen; ++i) {
    frame.payload.push_back(buffer[offset + i]);
  }

  return frame;
}

void Websocket::sendWebsocketFrame(Frame frame) {
  // Compute buffer size.
  uint64_t payloadLen = frame.payload.size();
  int payloadLenSize;
  if (payloadLen < 126) {
    payloadLenSize = 1;
  } else if (payloadLen < 0x8000) {
    payloadLenSize = 3;
  } else {
    payloadLenSize = 9;
  }
  size_t bufferSize = 1 + payloadLenSize + frame.masked * 4 + payloadLen;

  unsigned char buffer[bufferSize];

  buffer[0] = (frame.fin << 3) | frame.opcode;
  if (payloadLen < 126) {
    buffer[1] = (payloadLen & 0x7F) | (frame.masked << 7);
  } else if (payloadLen < 0x8000) {
    buffer[1] = (frame.masked << 7) | 0x7E;
    buffer[2] = payloadLen & 0xFF00;
    buffer[3] = payloadLen & 0xFF;
  } else {
    buffer[1] = (frame.masked << 7) | 0x7F;
    for (int i = 0; i < 8; ++i) {
      buffer[2 + i] = payloadLen & (0xFF << 8 * (7 - i));
    }
  }
  ssize_t offset = 1 + payloadLenSize;
  if (frame.masked) {
    // Mask payload with temporarily fixed masking-key.
    char mask[4] = {1, 2, 3, 4};
    for (int i = 0; i < 4; ++i) {
      buffer[offset + i] = mask[i];
    }
    offset += 4;

    // Append payload.
    for (int i = 0; i < payloadLen; ++i) {
      buffer[offset + i] = frame.payload[i] ^ mask[i % 4];
    }
  } else {
    memcpy(buffer + offset, frame.payload.data(), payloadLen);
  }

  // Send frame to the server.
  SSL_write(ssl_, buffer, bufferSize);
}

void Websocket::pong(Frame frame) {
  frame.opcode = Opcode::PONG_FRAME;
  sendWebsocketFrame(frame);
}

void Websocket::streamLoop() {
  char buffer[1 << 16];
  size_t bytes;

  // Keep receving data.
  while (true) {
    bytes = SSL_read(ssl_, buffer, sizeof(buffer));
    if (bytes <= 0) {
      std::cout << "Disconnected\n";
      break;
    }
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time_t);
    std::cout << "Time: " << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count() %
              1000;
    std::cout << "." << std::setw(3) << std::setfill('0') << ms << std::endl;
    std::cout << "\033[31mFrame size: " << bytes << "\033[0m\n";

    // Parse websocket frame.
    Frame frame = parseWebsocketFrame(buffer, bytes);
    if (frame.opcode == Opcode::PING_FRAME) {
      std::cout << "\033[31mPing Frame\033[0m\n";
      std::thread(&Websocket::pong, this, frame).detach();
    }
    if (!frame.payload.empty()) {
      std::cout << frame.payload << std::endl;
    }
  }
}

void Websocket::subscribe(const std::string& stream) {
  subscribe(std::vector<std::string>{stream});
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

  char buffer[1 << 16];
  int bytes = SSL_read(ssl_, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    throw std::runtime_error("Failed receving websocket handshake response");
  }
  buffer[bytes] = '\0';

  std::string response(buffer, bytes);
  std::cout << response << std::endl;
  if (response.find("HTTP/1.1 101 Switching Protocols") == std::string::npos) {
    throw std::runtime_error("Websocket handshake failed");
  }

  std::thread(&Websocket::streamLoop, this).join();
}

}  // namespace netkit