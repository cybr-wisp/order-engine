cmake_minimum_required(VERSION 3.20)

project(
    OrderEngine
    VERSION 0.1.0
    DESCRIPTION "A deterministic C++ limit-order-book and matching engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    order-engine
    src/apps/exchange_main.cpp
)

target_include_directories(
    order-engine
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

include(cmake/CompilerWarnings.cmake)
order_engine_set_warnings(order-engine)
