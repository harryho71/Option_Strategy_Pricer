cmake_minimum_required(VERSION 3.15)
project(OptionStrategyPricer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Core Library Sources
# ============================================================================

set(CORE_SOURCES
    src/cpp/src/models/BlackScholes.cpp
    src/cpp/src/options/EuropeanOption.cpp
    src/cpp/src/options/AmericanOption.cpp
    src/cpp/src/strategy/Strategy.cpp
    src/cpp/src/strategy/BullCall.cpp
    src/cpp/src/strategy/IronCondor.cpp
    src/cpp/src/api/RestServer.cpp
    src/cpp/src/api/PricingEndpoint.cpp
)

# ============================================================================
# Test Runner
# ============================================================================

add_executable(test_runner
    ${CORE_SOURCES}
    tests/cpp/test_blackscholes.cpp
)

target_include_directories(test_runner PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/cpp/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/tests/cpp
    ${CMAKE_SOURCE_DIR}/tests/cpp/fixtures
)

# ============================================================================
# Pricing Server (with cpp-httplib header-only library)
# ============================================================================

add_executable(pricing_server
    ${CORE_SOURCES}
    src/cpp/src/main_server.cpp
)

target_include_directories(pricing_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/cpp/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/httplib
)

# Enable HTTP server
target_compile_definitions(pricing_server PRIVATE ENABLE_HTTP_SERVER)

# Link Windows socket libraries for networking
if(WIN32)
    target_link_libraries(pricing_server PRIVATE ws2_32)
endif()
