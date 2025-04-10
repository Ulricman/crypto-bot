cmake_minimum_required(VERSION 3.28.3)
project(crypto-bot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)

set(OPENSSL_ROOT_DIR "/usr/local/openssl")
find_package(OpenSSL REQUIRED)

add_subdirectory("${CMAKE_SOURCE_DIR}/json")

set(netkitIncludeDir ${CMAKE_SOURCE_DIR}/netkit/include)
set(cexkitIncludeDir ${CMAKE_SOURCE_DIR}/cexkit/include)
include_directories(${netkitIncludeDir} ${cexkitIncludeDir})
file(GLOB Sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/*.cpp)
file(GLOB netkitSources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/netkit/src/*.cpp)
file(GLOB_RECURSE cexkitSources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/cexkit/src/*.cpp)

foreach(Source ${Sources})
    get_filename_component(Exec ${Source} NAME_WE)
    add_executable(${Exec} ${Source} ${netkitSources} ${cexkitSources})
    target_include_directories(${Exec} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${Exec} PUBLIC OpenSSL::SSL OpenSSL::Crypto nlohmann_json::nlohmann_json)
endforeach()