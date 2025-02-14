cmake_minimum_required(VERSION 3.16)
project(LibLocalJStack LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/archive/f5e54359df4c26b6230fc61d38aa294581393084.zip
    URL_HASH SHA256=17c535ec9abe0ea6d1e150875627c585f4b1293b769670da013ce24208726167
)
FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/v1.12.0.tar.gz
    URL_HASH SHA256=4dccf2d10f410c1e2feaff89966bfc49a1abb29ef6f08246335b110e001e09a9
)

FetchContent_MakeAvailable(fmt spdlog)

file(GLOB LibLocalJStack_SOURCES *.cpp)
file(GLOB LibLocalJStack_HEADERS *.h)

add_library(LibLocalJStack SHARED ${LibLocalJStack_SOURCES} ${LibLocalJStack_HEADERS})

# link jni
set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
message(STATUS ${JNI_INCLUDE_DIRS})

set_target_properties(LibLocalJStack PROPERTIES OUTPUT_NAME "localjstack")
set_target_properties(LibLocalJStack PROPERTIES VERSION 0.1 SOVERSION 0)

LIST(APPEND TD_LIBS
    fmt::fmt
    spdlog::spdlog_header_only
)

target_link_libraries(LibLocalJStack PRIVATE ${TD_LIBS})
