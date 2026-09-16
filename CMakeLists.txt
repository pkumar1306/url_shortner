cmake_minimum_required(VERSION 3.20)

project(url_shortener)

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
    src/utils/rate_limiter.cpp
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
