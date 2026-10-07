cmake_minimum_required(VERSION 3.15)

project(bindings_dart LANGUAGES CXX)

add_library(bindings_dart SHARED
    sourcetrail_db_bindings.cpp
    sourcetrail_db_bindings.h
)

target_compile_features(bindings_dart PRIVATE cxx_std_17)

target_include_directories(bindings_dart PRIVATE
    "${CMAKE_SOURCE_DIR}/core/include"
    "${CORE_BINARY_DIR}/include"
)

target_link_libraries(bindings_dart PRIVATE ${LIB_CORE_TARGET_NAME})

set_target_properties(bindings_dart PROPERTIES
    OUTPUT_NAME "sourcetrail_db_bindings"
)
