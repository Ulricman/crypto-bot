#ifndef __NETKIT_UTILS_HPP__
#define __NETKIT_UTILS_HPP__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace netkit {

// Hostname resolution.
std::string resolveHostname(const char* hostname);
std::string resolveHostname(const std::string& hostname);

// * OpenSSL related helper function.

// Initialize OpenSSL library.
void initOpenssl();

// Cleanup OpenSSL resources.
void cleanupOpenssl();

SSL_CTX* createSSLContext(const char* caPath);

// Other helper functions.

std::string getTimestamp();

std::string encryptWithHMAC(const char* key, const char* data);

std::string getSignature(const std::string& key, const std::string& data);

}  // namespace netkit

#endif