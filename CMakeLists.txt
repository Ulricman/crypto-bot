cmake_minimum_required(VERSION 3.28.3)
project(crypto-bot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENSSL_ROOT_DIR "/usr/local/openssl")
find_package(OpenSSL REQUIRED)

add_executable(
    main main.cpp
)
target_include_directories(
    main PUBLIC ${OPENSSL_INCLUDE_DIR}
)
target_link_libraries(
    main PUBLIC OpenSSL::SSL OpenSSL::Crypto
)