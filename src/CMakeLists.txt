project(libsimple_inference_server)


file(GLOB source *.cpp)
add_library(libsimple_inference_server STATIC ${source})
target_link_libraries(libsimple_inference_server drogon magic_enum::magic_enum)
