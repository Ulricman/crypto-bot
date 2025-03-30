#include "netkit/netdef.hpp"

namespace netkit {

std::ifstream& operator>>(std::ifstream& file, Config& config) {
  if (!file.is_open()) {
    throw std::runtime_error("Config file is not opened correctly");
  }

  nlohmann::json configFile;
  file >> configFile;

  if (configFile.contains("api_key")) {
    config.apiKey = configFile["api_key"];
  } else {
    throw std::runtime_error("Config file does not contain api_key");
  }

  if (configFile.contains("api_secret")) {
    config.apiSecret = configFile["api_secret"];
  } else {
    throw std::runtime_error("Config file does not contain api_secret");
  }

  if (configFile.contains("ca_path")) {
    config.caPath = configFile["ca_path"];
  } else {
    throw std::runtime_error("Config file does not contain ca_path");
  }

  if (configFile.contains("proxy_host")) {
    config.proxyHost = configFile["proxy_host"];
  }

  if (configFile.contains("proxy_port")) {
    config.proxyPort = configFile["proxy_port"];
  }

  return file;
}
}  // namespace netkit