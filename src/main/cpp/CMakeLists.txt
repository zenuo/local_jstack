project(LibLocalJStack LANGUAGES CXX)

file(GLOB LibLocalJStack_SOURCES *.cpp)
file(GLOB LibLocalJStack_HEADERS *.h)

add_library(LibLocalJStack SHARED ${LibLocalJStack_SOURCES} ${LibLocalJStack_HEADERS})

if(MSVC)
    target_link_options(LibLocalJStack PRIVATE /MAP:localjstack.map /MAPINFO:EXPORTS)
endif()

# 设置头文件路径
target_include_directories(LibLocalJStack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# link jni
set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
message(STATUS ${JNI_INCLUDE_DIRS})

set_target_properties(LibLocalJStack PROPERTIES OUTPUT_NAME "localjstack")
set_target_properties(LibLocalJStack PROPERTIES VERSION 0.1 SOVERSION 0)
