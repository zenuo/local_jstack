#include "app_LocalJStack.h"
#include "jvm_def.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <dlfcn.h>
#endif

typedef jint (*ThreadDumpFunc)(AttachOperation *, outputStream *);

static ThreadDumpFunc thread_dump = nullptr;

#ifndef __APPLE__
uintptr_t get_libjvm_base_address(pid_t pid)
{
    std::stringstream maps_path;
    maps_path << "/proc/" << pid << "/maps";
    std::ifstream maps_file(maps_path.str());
    std::string line;
    while (std::getline(maps_file, line))
    {
        if (line.find("libjvm.so") != std::string::npos)
        {
            std::stringstream line_stream(line);
            std::string address_range;
            std::getline(line_stream, address_range, ' ');
            std::stringstream address_stream(address_range);
            std::string start_address_str;
            std::getline(address_stream, start_address_str, '-');
            return std::stoull(start_address_str, nullptr, 16);
        }
    }
    return 0;
}
#endif

#ifdef __APPLE__
uintptr_t get_libjvm_base_address(pid_t pid)
{
    // 获取当前进程中加载的动态库数量
    uint32_t count = _dyld_image_count();

    for (uint32_t i = 0; i < count; i++) {
        // 获取动态库的路径
        const char* imageName = _dyld_get_image_name(i);
        if (imageName == nullptr) {
            continue;
        }

        // 检查动态库名称是否匹配
        std::string currentImageName(imageName);
        if (currentImageName.find("libjvm.dylib") != std::string::npos) {
            // 返回动态库的基地址
            return (uintptr_t)_dyld_get_image_header(i);
        }
    }

    // 如果未找到，返回 0
    return 0;
}
#endif

JNIEXPORT void JNICALL Java_app_LocalJStack_init(JNIEnv *env, jclass cls, jlong threadDumpOffset)
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^---%L---%$] [pid %P thread %t] %v");

    pid_t pid = getpid();
    uintptr_t libjvm_base = get_libjvm_base_address(pid);
    uintptr_t threadDumpAddress = threadDumpOffset + libjvm_base;

    spdlog::info("libjvm_base:{},threadDumpAddress:{}", libjvm_base, threadDumpAddress);

    thread_dump = (ThreadDumpFunc)threadDumpAddress;
}

JNIEXPORT void JNICALL Java_app_LocalJStack_dumpStack(JNIEnv *env, jclass cls, jobject writer)
{
    if (thread_dump == nullptr)
    {
        spdlog::error("thread_dump not init");
        exit(1);
    }

    AttachOperation ao = AttachOperation((char *)"thread_dump");
    ao.set_arg(0, (char*)"le");

    bufferedStream buf;

    jint ret = thread_dump(&ao, &buf);

    char *dump_result = buf.as_string();

    spdlog::info("dump_result, len:{}, buf.size:{}", strlen(dump_result), buf.size());

    jclass writerClass = env->GetObjectClass(writer);

    if (writerClass == NULL)
    {
        spdlog::error("writer class not found");
        return;
    }

    // 获取 write(char[],int,int) 方法的 methodID
    jmethodID writeMethodID = env->GetMethodID(writerClass, "write", "([CII)V");
    if (writeMethodID == NULL)
    {
        spdlog::error("append method not found");
        return;
    }

    jcharArray jResult = env->NewCharArray(buf.size());

    jchar* resultChars = (jchar*)malloc(buf.size() * sizeof(jchar));
    for (size_t i = 0; i < buf.size(); i++) {
        resultChars[i] = (jchar)dump_result[i];
    }

    env->SetCharArrayRegion(jResult, 0, buf.size(), resultChars);

    env->CallVoidMethod(writer, writeMethodID, jResult, 0, buf.size());

    free(resultChars);
}
