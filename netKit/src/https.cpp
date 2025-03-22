#include "netKit/https.hpp"

namespace netkit {

Agent::Agent(const std::string& hostname, const unsigned int port,
             const char* caPath, const std::string& proxyHostname,
             const unsigned int proxyPort)
    : hostname_(hostname),
      port_(port),
      proxyHostname_(proxyHostname),
      proxyPort_(proxyPort) {
  // Create socket or proxy tunnel if proxy is given.
  // TODO: requests without proxt needs to be checked.
  if (!proxyHostname_.empty()) {
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
  ctx_ = createSSLContext(caPath);

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

std::string Agent::request(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& params) {
  std::ostringstream oss;
  oss << "GET " << url;

  // Append params to url.
  if (!params.empty()) {
    oss << "?";
    for (auto it = params.begin(); it != params.end(); ++it) {
      if (it != params.begin()) {
        oss << "&";
      }
      oss << it->first << "=" << it->second;
    }
  }

  oss << " HTTP/1.1\r\nHost: " << hostname_ << "\r\n"
      << "User-Agent: OpenSSL/1.1.1\r\nConnection: close\r\n\r\n";

  // Send request.
  std::string msg = oss.str();
  SSL_write(ssl_, msg.data(), msg.size());

  // Receive response.
  std::string response;
  char buffer[1024];
  ssize_t bytes;
  while ((bytes = SSL_read(ssl_, buffer, sizeof(buffer))) > 0) {
    response.append(buffer, bytes);
  }

  // Skip the response header.
  auto header = response.find("\r\n\r\n");
  if (header != std::string::npos) {
    response = response.substr(header + 4);
  }

  return response;
}

Agent::~Agent() {
  // Cleanup OpenSSL resources.
  SSL_shutdown(ssl_);
  SSL_free(ssl_);
  close(sockFd_);
  SSL_CTX_free(ctx_);
  cleanupOpenssl();
}

}  // namespace netkit