project(LocalJStackTest LANGUAGES CXX)

# only test for windows
if(MSVC)
    find_package(Java REQUIRED)
    find_package(JNI REQUIRED)
    include_directories(${JNI_INCLUDE_DIRS})

    aux_source_directory(. DIR_SRCS)
    add_executable(LocalJStackTest main.cpp)

    target_include_directories(LocalJStackTest PUBLIC LibLocalJStack ${JNI_INCLUDE_DIRS})
    target_link_libraries(LocalJStackTest LibLocalJStack ${JNI_LIBRARIES})
endif()
