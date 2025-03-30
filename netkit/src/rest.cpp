#include "netkit/rest.hpp"

namespace netkit {

Rest::Rest(const std::string &host, const unsigned int port,
           const Config &config)
    : host_(host),
      port_(port),
      apiKey_(config.apiKey),
      apiSecret_(config.apiSecret),
      proxyHost_(config.proxyHost),
      proxyPort_(config.proxyPort) {
  // Create socket or proxy tunnel if proxy is given.
  // TODO: requests without proxt needs to be checked.
  if (!proxyHost_.empty()) {
    sockFd_ = proxyTunnel(proxyHost_, proxyPort_, host_, port_);
  } else {
    // Resolve IP of the hostname.
    std::string ip = resolveHostname(host);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, ip.data(), &serverAddress.sin_addr);

    sockFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd_ < 0) {
      throw std::runtime_error(std::string("Failed creating socket: ") +
                               std::strerror(errno));
    }

    if (connect(sockFd_, reinterpret_cast<struct sockaddr *>(&serverAddress),
                sizeof(serverAddress)) < 0) {
      close(sockFd_);
      throw std::runtime_error(std::string("Failed connecting to proxy: ") +
                               std::strerror(errno));
    }
  }

  // Initialize OpenSSL and define SSL context.
  initOpenssl();
  ctx_ = createSSLContext(config.caPath.data());

  // Create SSL object and bind to socket.
  ssl_ = SSL_new(ctx_);
  SSL_set_fd(ssl_, sockFd_);
  SSL_set_tlsext_host_name(ssl_, host_.data());

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

Rest::~Rest() {
  // Cleanup OpenSSL resources.
  SSL_shutdown(ssl_);
  SSL_free(ssl_);
  close(sockFd_);
  SSL_CTX_free(ctx_);
  cleanupOpenssl();
}

std::string Rest::executeRequest(
    const std::string &url, const std::string &httpMethod,
    const std::unordered_map<std::string, std::string> &headers) {
  std::ostringstream oss;

  // Perpare HTTPS request message.
  oss << httpMethod << " " << url << " HTTP/1.1\r\n";
  oss << "Host: " << host_ << "\r\n";
  for (const auto &header : headers) {
    oss << header.first << ":" << header.second << "\r\n";
  }
  oss << "User-Agent: OpenSSL/1.1.1\r\nConnection: keep-alive\r\n\r\n";

  // Send HTTPS request.
  std::string message = oss.str();
  if (SSL_write(ssl_, message.data(), message.size()) <= 0) {
    std::cerr << "Failed sending HTTPS request\n";
    throw std::runtime_error("Failed sending HTTPS request\n");
  }

  // Receive HTTPS response.
  std::string response;
  char buffer[1024];
  ssize_t bytes;
  int contentLength = -1;
  while ((bytes = SSL_read(ssl_, buffer, sizeof(buffer))) > 0) {
    response.append(buffer, bytes);
    if (contentLength == -1) {
      // Get the lengthe of the response body to determine the end of response.
      auto headerIt = response.find("\r\n\r\n");
      if (headerIt != std::string::npos) {
        auto contentLengthIt = response.find("Content-Length");
        std::istringstream iss(response.substr(contentLengthIt + 16));
        iss >> contentLength;

        // Remove the header.
        response = response.substr(headerIt + 4);
      }
    } else if (response.size() == contentLength) {
      break;
    }
  }

  return response;
}

std::string Rest::joinParams(
    const std::unordered_map<std::string, std::string> &params) {
  std::string query;
  for (auto it = params.cbegin(); it != params.cend(); ++it) {
    if (it != params.cbegin()) {
      query += "&";
    }
    query += it->first + "=" + it->second;
  }
  return query;
}

std::string Rest::sendPublicRequest(
    std::string url, const std::string &httpMethod,
    const std::unordered_map<std::string, std::string> &params) {
  std::ostringstream oss;

  // Append params to url.
  if (!params.empty()) {
    url += "?" + joinParams(params);
  }

  return executeRequest(url, httpMethod);
}

std::string Rest::sendSignedRequest(
    std::string url, const std::string &httpMethod,
    const std::unordered_map<std::string, std::string> &params) {
  // Append the parameters to the url.
  // Note that the url here does not contain host IP, but pure path.
  url += "?";
  std::string timestamp = getTimestamp(), query;

  // Append params to url.
  if (params.empty()) {
    query = "timestamp=" + timestamp;
  } else {
    query = joinParams(params) + "&timestamp=" + timestamp;
  }
  url += query;
  // Append signature to url.
  std::string signature = getSignature(apiSecret_, query);
  url += "&signature=" + signature;

  // Define headers.
  std::unordered_map<std::string, std::string> headers{
      {"X-MBX-APIKEY", apiKey_}};

  return executeRequest(url, httpMethod, headers);
}

}  // namespace netkit