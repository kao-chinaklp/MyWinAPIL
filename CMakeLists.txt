cmake_minimum_required(VERSION 3.29)
project(MyWinAPIL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "source/*.cpp")

set(CMAKE_AR gcc-ar)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static-libstdc++ -static-libgcc")

add_library(MyWinAPIL STATIC ${SOURCES} library.cpp)

target_include_directories(MyWinAPIL PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Main configurations
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MyWinAPIL PRIVATE DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
else()
    target_compile_definitions(MyWinAPIL PRIVATE NDEBUG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -s -flto")
endif()