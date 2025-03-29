#include "cexkit/utils.hpp"

namespace cexkit {

std::string upper(const std::string& token) {
  std::string upperToken;
  for (char c : token) {
    upperToken += std::toupper(c);
  }
  return upperToken;
}

}  // namespace cexkit