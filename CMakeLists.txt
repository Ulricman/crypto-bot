cmake_minimum_required(VERSION 3.28.3)
project(crypto-bot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENSSL_ROOT_DIR "/usr/local/openssl")
find_package(OpenSSL REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/json")
add_subdirectory("${CMAKE_SOURCE_DIR}/curl")

file(GLOB Sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")

foreach(Source ${Sources})
    get_filename_component(Exec ${Source} NAME_WE)
    add_executable(${Exec} ${Source})
    target_include_directories(${Exec} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${Exec} PUBLIC OpenSSL::SSL OpenSSL::Crypto CURL::libcurl)
endforeach()

add_subdirectory("${CMAKE_SOURCE_DIR}/netKit")
add_subdirectory("${CMAKE_SOURCE_DIR}/cexKit")