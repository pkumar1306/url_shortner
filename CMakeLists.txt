cmake_minimum_required(VERSION 3.20)

project(url_shortener)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(url_shortener
    src/main.cpp
    src/config/app_config.cpp
    src/database/database.cpp
    src/handlers/url_handler.cpp
    src/services/auth_service.cpp
    src/services/url_service.cpp
    src/url_store.cpp
    src/utils/auth_utils.cpp
    src/utils/logger.cpp
    src/utils/rate_limiter.cpp
    src/utils/request_id.cpp
    src/utils/url_utils.cpp
)

target_include_directories(url_shortener
    PRIVATE include
)

target_link_libraries(url_shortener
    PRIVATE Drogon::Drogon OpenSSL::Crypto
)

# On Windows with a dynamic vcpkg triplet, place Drogon and its dependency DLLs
# beside the executable so it can be launched directly from the build folder.
if(WIN32)
    add_custom_command(TARGET url_shortener POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${Drogon_DIR}/../../bin"
            "$<TARGET_FILE_DIR:url_shortener>"
        COMMAND_EXPAND_LISTS
    )
endif()

add_executable(url_store_tests
    tests/url_store_tests.cpp
    src/url_store.cpp
)
target_include_directories(url_store_tests PRIVATE include)
add_test(NAME url_store_tests COMMAND url_store_tests)

add_executable(rate_limiter_test
    tests/rate_limiter_test.cpp
    src/utils/rate_limiter.cpp
)
target_include_directories(rate_limiter_test PRIVATE include)
add_test(NAME rate_limiter_test COMMAND rate_limiter_test)

add_executable(auth_utils_tests
    tests/auth_utils_tests.cpp
    src/utils/auth_utils.cpp
)
target_include_directories(auth_utils_tests PRIVATE include)
target_link_libraries(auth_utils_tests PRIVATE OpenSSL::Crypto)
add_test(NAME auth_utils_tests COMMAND auth_utils_tests)


add_executable(logger_tests
    tests/logger_tests.cpp
    src/utils/logger.cpp
)

target_include_directories(logger_tests PRIVATE include)

add_test(NAME logger_tests COMMAND logger_tests)