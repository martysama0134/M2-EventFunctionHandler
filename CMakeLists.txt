cmake_minimum_required(VERSION 3.20)
project(EventFunctionHandler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(event_handler_test
    EventFunctionHandler.cpp
    test/test_main.cpp
    test/test_seconds.cpp
    test/test_pulse.cpp
    test/test_safety.cpp
    test/test_query.cpp
    test/test_handles.cpp
    test/test_mixed.cpp
    test/test_scale.cpp
    test/test_compat.cpp
)

target_include_directories(event_handler_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/test/stubs
    ${CMAKE_SOURCE_DIR}/test
)

enable_testing()
add_test(NAME event_handler_test COMMAND event_handler_test)
