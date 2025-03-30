#ifndef __NETKIT_NETDEF_HPP__
#define __NETKIT_NETDEF_HPP__

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace netkit {

struct Config {
  std::string apiKey;
  std::string apiSecret;
  std::string caPath;
  std::string proxyHost;
  int proxyPort = 0;

};  // Config

std::ifstream& operator>>(std::ifstream& file, Config& config);
}  // namespace netkit

#endif