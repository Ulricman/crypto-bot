#ifndef __NETKIT_PROXY_HPP__
#define __NETKIT_PROXY_HPP__

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>

#include <cstring>
#include <string>

#include "netKit/utils.hpp"

namespace netkit {

// Establish a tunnel with the proxy server and return the socket fd.
int proxyTunnel(const std::string& proxyHostname, unsigned int proxyPort,
                const std::string& hostname, unsigned int port);
int proxyTunnel(const char* proxyHostname, unsigned int proxyPort,
                const char* hostname, unsigned int port);

}  // namespace netkit

#endif