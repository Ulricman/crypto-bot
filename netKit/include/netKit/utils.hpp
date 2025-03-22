#ifndef __NETKIT_UTILS_HPP__
#define __NETKIT_UTILS_HPP__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace netkit {

std::string resolveHostname(const char* hostname);
std::string resolveHostname(const std::string& hostname);

}  // namespace netkit

#endif