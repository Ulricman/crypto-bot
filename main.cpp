#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

const std::string api_key =
    "KiuKcWHgGD16UkX5JOS6hdkkl84fxHkKnKSA6j16bGnm0qHFsdTbK3fZoKnGJ0pj";
const std::string secret_key =
    "7nz1JfEHEZ2GoNuLemwBXirEEpwZtYMgpmw75XlaKTBeBMzfmYevH92Bvtdygiwn";
const std::string SpotHost = "https://api.binance.com";

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  static_cast<std::string *>(userp)->append(static_cast<const char *>(contents),
                                            size * nmemb);
  return size * nmemb;
}

void request(std::string &url) {
  CURL *curl;
  curl = curl_easy_init();
  std::string buffer;
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1");
    curl_easy_setopt(curl, CURLOPT_PROXYPORT, 20112);
    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
    } else {
      std::cout << "Response: " << buffer << std::endl;
    }
    curl_easy_cleanup(curl);
  } else {
    std::cerr << "curl is empty\n";
  }
}

int main() {
  std::string url = SpotHost + "/api/v3/time";
  request(url);
}