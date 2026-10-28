cmake_minimum_required(VERSION 3.14)

project(mudo_gm
    VERSION 0.1.0
    DESCRIPTION "muduo 风格的 C++ 基础/网络库"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "未指定构建类型，默认使用 Debug")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /bigobj)
else()
    add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor
                        -Wold-style-cast -Woverloaded-virtual)
endif()

option(SANITIZE "Enable AddressSanitizer" OFF)
if(SANITIZE)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

option(BUILD_TESTS    "构建单元测试" OFF)
option(BUILD_EXAMPLES "构建示例代码" OFF)

file(GLOB MUDO_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB MUDO_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

find_package(Threads REQUIRED)

add_library(${PROJECT_NAME} STATIC ${MUDO_SOURCES} ${MUDO_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

list(LENGTH MUDO_SOURCES MUDO_SOURCE_COUNT)
message(STATUS "以静态库方式构建 ${PROJECT_NAME}，源文件数：${MUDO_SOURCE_COUNT}")

if(BUILD_TESTS)
    enable_testing()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

if(BUILD_EXAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
        add_subdirectory(examples)
    endif()
endif()
