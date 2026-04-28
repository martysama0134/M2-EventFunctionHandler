cmake_minimum_required(VERSION 3.20)
project(EventFunctionHandler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(event_handler_test
    EventFunctionHandler.cpp
    test/main.cpp
)

target_include_directories(event_handler_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/test/stubs
)

enable_testing()
add_test(NAME event_handler_test COMMAND event_handler_test)
