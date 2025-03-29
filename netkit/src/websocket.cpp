#include "netkit/websocket.hpp"

namespace netkit {

Websocket::Websocket(const std::string& hostname, const unsigned int port,
                     const std::string& caPath, const std::string& apiKey,
                     const std::string& apiSecret, const std::string& endpoint,
                     const std::string& proxyHostname,
                     const unsigned int proxyPort)
    : hostname_(hostname),
      port_(port),
      endpoint_(endpoint),
      apiKey_(apiKey),
      apiSecret_(apiSecret),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort),
      streamLoop_(nullptr) {
  // Create socket or proxy tunnel if proxy is given.
  if (!proxyHostname.empty()) {
    sockFd_ = proxyTunnel(proxyHostname_, proxyPort_, hostname_, port_);
  } else {
    // Resolve IP of the hostname.
    std::string ip = resolveHostname(hostname);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr);

    sockFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ < 0) {
      throw std::runtime_error(std::string("Failed creating socket: ") +
                               std::strerror(errno));
    }

    if (connect(sockFd_, reinterpret_cast<struct sockaddr*>(&serverAddress),
                sizeof(serverAddress)) < 0) {
      close(sockFd_);
      throw std::runtime_error(std::string("Failed connecting to proxy: ") +
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
  connectEndpoint(endpoint_);
  streamLoop_ = new std::thread(&Websocket::streamLoop, this);
}

Websocket::~Websocket() {
  if (streamLoop_ && streamLoop_->joinable()) {
    streamLoop_->join();
  }

  // Cleanup OpenSSL resources.
  SSL_shutdown(ssl_);
  SSL_free(ssl_);
  close(sockFd_);
  SSL_CTX_free(ctx_);
  cleanupOpenssl();
}

void Websocket::join() {
  if (streamLoop_ && streamLoop_->joinable()) {
    streamLoop_->join();
  }
}

Frame Websocket::parseWebsocketFrame(const char* buffer, size_t len) {
  // For more detailed information about frame structure, refer to RFC6455:
  // https://datatracker.ietf.org/doc/html/rfc6455#autoid-21:~:text=).%0A%0A5.2.-,Base%20Framing%20Protocol,-This%20wire%20format
  const unsigned char* data = reinterpret_cast<const unsigned char*>(buffer);
  bool fin = (data[0] & 0x80) != 0;
  Opcode opcode = static_cast<Opcode>(data[0] & 0x0F);
  bool masked = data[1] & 0x80;
  size_t payloadLen = data[1] & 0x7F;

  if (masked) {
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

  Frame frame(fin, masked, opcode, &buffer[offset], payloadLen);

  return frame;
}

void Websocket::sendWebsocketFrame(Frame frame) {
  // Compute buffer size.
  uint64_t payloadLen = frame.payload.size();
  bool masked = frame.masked;
  int payloadLenSize;
  if (payloadLen < 126) {
    payloadLenSize = 1;
  } else if (payloadLen < 0x8000) {
    payloadLenSize = 3;
  } else {
    payloadLenSize = 9;
  }
  size_t bufferSize = 1 + payloadLenSize + masked * 4 + payloadLen;

  unsigned char buffer[bufferSize];

  buffer[0] = (frame.fin << 7) | frame.opcode;
  if (payloadLen < 126) {
    buffer[1] = (payloadLen & 0x7F) | (masked << 7);
  } else if (payloadLen < 0x8000) {
    buffer[1] = (masked << 7) | 0x7E;
    buffer[2] = payloadLen & 0xFF00;
    buffer[3] = payloadLen & 0xFF;
  } else {
    buffer[1] = (masked << 7) | 0x7F;
    for (int i = 0; i < 8; ++i) {
      buffer[2 + i] = payloadLen & (0xFF << 8 * (7 - i));
    }
  }
  ssize_t offset = 1 + payloadLenSize;
  if (masked) {
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
    memcpy(&buffer[offset], frame.payload.data(), payloadLen);
  }

  // Send frame to the server.
  SSL_write(ssl_, buffer, bufferSize);
}

void Websocket::pong(std::string payload) {
  Frame frame(true, true, Opcode::PONG_FRAME, std::move(payload));

  sendWebsocketFrame(std::move(frame));
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
    std::cout << "Frame size: " << bytes << "\n";

    // Parse websocket frame.
    Frame frame = parseWebsocketFrame(buffer, bytes);
    if (frame.opcode == Opcode::PING_FRAME) {
      std::cout << "Ping Frame\n";
      std::thread(&Websocket::pong, this, frame.payload).detach();
    }
    if (!frame.payload.empty()) {
      // Parse frame and call callback function if registered.
      nlohmann::json payload = nlohmann::json::parse(frame.payload);
      if (payload.contains("stream")) {
        std::string stream = payload["stream"];
        if (callbacks_.contains(stream)) {
          callbacks_[stream](std::move(frame));
        }
      }
    }
  }
}

bool Websocket::connectEndpoint(const std::string& endpoint) {
  std::string wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

  // Fill upgrade request message.
  std::ostringstream oss;
  oss << "GET " << endpoint << " HTTP/1.1\r\n";
  oss << "Host: " << hostname_ << "\r\n";
  oss << "Upgrade: websocket\r\n";
  oss << "Connection: Upgrade\r\n";
  oss << "Sec-WebSocket-Key: " << wsKey << "\r\n";
  oss << "Sec-WebSocket-Version: 13\r\n\r\n";

  // Send upgrade request to perform websocket handshake.
  std::string upgradeRequest = oss.str();
  if (SSL_write(ssl_, upgradeRequest.data(), upgradeRequest.size()) <= 0) {
    std::cerr << "Failed sending upgrade request\n";
    return false;
  }

  // Receive websocket handshake response.
  char buffer[1 << 8];
  int bytes = SSL_read(ssl_, buffer, sizeof(buffer) - 1);
  if (bytes <= 0) {
    std::cerr << "Failed receving websocket handshake response\n";
    return false;
  }
  buffer[bytes] = '\0';

  std::string response(buffer, bytes);
  std::cout << response << std::endl;

  // If server refuses to upgrade, return false.
  if (response.find("HTTP/1.1 101 Switching Protocols") == std::string::npos) {
    std::cerr << "Websocket handshake failed\n";
    return false;
  }

  return true;
}

void Websocket::subscribe(const std::string& stream) {
  subscribe(std::vector<std::string>{stream});
}

void Websocket::subscribe(const std::vector<std::string>& streams) {
  nlohmann::json request;
  request["method"] = "SUBSCRIBE";
  request["params"] = streams;
  request["id"] = 1;

  std::string payload = request.dump();
  std::cout << "subscribe: " << payload << std::endl;
  Frame frame{true, true, Opcode::TEXT_FRAME, std::move(payload)};

  sendWebsocketFrame(std::move(frame));
}

void Websocket::unsubscribe(const std::vector<std::string>& streams) {
  nlohmann::json request;
  request["method"] = "UNSUBSCRIBE";
  request["params"] = streams;
  request["id"] = 312;

  std::string payload = request.dump();
  std::cout << payload << std::endl;
  Frame frame(true, true, Opcode::TEXT_FRAME, std::move(payload));

  sendWebsocketFrame(std::move(frame));
}

void Websocket::unsubscribe(const std::string& stream) {
  unsubscribe(std::vector<std::string>{stream});
}

void Websocket::listSubscriptions() {
  nlohmann::json request;
  request["method"] = "LIST_SUBSCRIPTIONS";
  request["id"] = 3;

  std::string payload = request.dump();
  Frame frame(true, true, Opcode::TEXT_FRAME, std::move(payload));

  sendWebsocketFrame(std::move(frame));
}

int Websocket::numStreams() const { return streams_.size(); }

void Websocket::registerCallback(const std::string& stream,
                                 const std::function<void(Frame)>& cb) {
  if (!cb) {
    throw std::runtime_error(
        std::string("Registering an invalid callback for stream ") + stream);
  }
  callbacks_[stream] = cb;
}

}  // namespace netkit