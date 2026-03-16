# cpp-httplib (HTTP server library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib/CMakeLists.txt")
    add_subdirectory(cpp-httplib)
else()
    message(FATAL_ERROR
        "third_party/cpp-httplib not found.\n"
        "Run scripts/download-deps.sh to download dependencies.")
endif()

# nlohmann-json (JSON library)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann-json/CMakeLists.txt")
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(nlohmann-json)
else()
    message(FATAL_ERROR
        "third_party/nlohmann-json not found.\n"
        "Run scripts/download-deps.sh to download dependencies.")
endif()

# Google Test (testing framework, only when tests are enabled)
if(BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        add_subdirectory(googletest)
    else()
        message(FATAL_ERROR
            "third_party/googletest not found.\n"
            "Run scripts/download-deps.sh to download dependencies.")
    endif()
endif()
