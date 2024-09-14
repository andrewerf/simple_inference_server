cmake_minimum_required(VERSION 3.27)
project(simple_inference_server)

set(CMAKE_CXX_STANDARD 23)

set(BUILD_ORM OFF CACHE BOOL "Build ORM")
set(USE_STATIC_LIBS_ONLY OFF CACHE BOOL "Use static libs only")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build drogon as a shared lib")
set(USE_SPDLOG ON CACHE BOOL "Use spdlog")
set(BUILD_BROTLI OFF CACHE BOOL "Build Brotli")
set(BUILD_YAML_CONFIG OFF CACHE BOOL "Build yaml config")
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_CTL "Build drogon_ctl" OFF)
add_subdirectory(3rdparty/drogon)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(simple_inference_server main.cpp)
target_link_libraries(simple_inference_server drogon Boost::program_options)
