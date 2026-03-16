cmake_minimum_required(VERSION 3.15)
project(smart-mobility-backend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Third-party dependencies (vendored for air-gapped environments) ---
# Run scripts/download-deps.sh to populate third_party/ before building.
add_subdirectory(third_party)

# --- Application ---
set(SOURCES
    src/VehicleState.cpp
    src/VehicleService.cpp
    src/VehicleResource.cpp
    src/main.cpp
)

add_executable(smart-mobility-backend ${SOURCES})

target_link_libraries(smart-mobility-backend
    PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

if(UNIX)
    target_link_libraries(smart-mobility-backend PRIVATE pthread)
endif()

install(TARGETS smart-mobility-backend RUNTIME DESTINATION bin)

# --- Coverage ---
option(ENABLE_COVERAGE "Enable gcov coverage" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    endif()
endif()

# --- Tests ---
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()

    set(LIB_SOURCES
        src/VehicleState.cpp
        src/VehicleService.cpp
        src/VehicleResource.cpp
    )

    add_executable(VehicleStateTest test/VehicleStateTest.cpp ${LIB_SOURCES})
    target_link_libraries(VehicleStateTest
        PRIVATE GTest::gtest_main httplib::httplib nlohmann_json::nlohmann_json)
    if(UNIX)
        target_link_libraries(VehicleStateTest PRIVATE pthread)
    endif()

    add_executable(VehicleServiceTest test/VehicleServiceTest.cpp ${LIB_SOURCES})
    target_link_libraries(VehicleServiceTest
        PRIVATE GTest::gtest_main httplib::httplib nlohmann_json::nlohmann_json)
    if(UNIX)
        target_link_libraries(VehicleServiceTest PRIVATE pthread)
    endif()

    add_executable(VehicleResourceTest test/VehicleResourceTest.cpp ${LIB_SOURCES})
    target_link_libraries(VehicleResourceTest
        PRIVATE GTest::gtest_main httplib::httplib nlohmann_json::nlohmann_json)
    if(UNIX)
        target_link_libraries(VehicleResourceTest PRIVATE pthread)
    endif()

    include(GoogleTest)
    gtest_discover_tests(VehicleStateTest)
    gtest_discover_tests(VehicleServiceTest)
    gtest_discover_tests(VehicleResourceTest)
endif()
