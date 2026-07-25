
# --- Configurations ---
cmake_minimum_required(VERSION 3.20)
project(cpp-order-book LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Engine library ---
add_library(engine
    src/core/order_book.cpp
    src/core/order.cpp
)
target_include_directories(engine PUBLIC include/)

if(MSVC)
    target_compile_options(engine PRIVATE /W4)
else()
    target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
# Is the compiler MSVC?
# ├── Yes → use /W4
# └── No  → use -Wall -Wextra -Wpedantic

# --- GoogleTest ---
# FetchContent allows your build to download and add external dependencies during the CMake configuration step
include(FetchContent)
FetchContent_Declare( googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest) # Tells CMake to make the declared dependency availible 

# --- Test executable ---
enable_testing()
add_executable(tests
    tests/unit/order_tests.cpp
    tests/unit/order_book_test.cpp
)
target_link_libraries(tests PRIVATE engine GTest::gtest_main)
add_test(NAME all_tests COMMAND tests)