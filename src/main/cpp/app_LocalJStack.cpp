#include "app_LocalJStack.h"
#include "jvm_def.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
// #include <sys/stat.h>


// 操作系统检测宏
#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #define OS_MACOS
#elif defined(__linux__)
    #define OS_LINUX
#else
    #error "Unknown operating system"
#endif

#ifdef OS_WINDOWS
#include <windows.h>
#include <psapi.h>
#elif defined(OS_MACOS)
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <dlfcn.h>
#elif defined(OS_LINUX)
#include <dlfcn.h>
#include <link.h>
#endif

typedef jint (*ThreadDumpFunc)(AttachOperation *, outputStream *);

static ThreadDumpFunc thread_dump = nullptr;

#ifdef OS_WINDOWS
uintptr_t getLibraryBaseAddress(const std::string& libraryName)
{
// 获取当前进程句柄
    HANDLE hProcess = GetCurrentProcess();

    // 枚举当前进程加载的模块
    HMODULE hModules[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
        for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char moduleName[MAX_PATH];
            // 获取模块的完整路径
            if (GetModuleFileNameExA(hProcess, hModules[i], moduleName, sizeof(moduleName))) {
                // 检查模块名称是否匹配
                std::string currentModuleName(moduleName);
                if (currentModuleName.find(libraryName) != std::string::npos) {
                    // 返回模块的基地址
                    return reinterpret_cast<uintptr_t>(hModules[i]);
                }
            }
        }
    }

    // 如果未找到，返回 0
    return 0;
}
#elif defined(OS_MACOS)
uintptr_t getLibraryBaseAddress(const std::string& libraryName)
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
        if (currentImageName.find(libraryName) != std::string::npos) {
            // 返回动态库的基地址
            return (uintptr_t)_dyld_get_image_header(i);
        }
    }

    // 如果未找到，返回 0
    return 0;
}
#elif defined(OS_LINUX)
uintptr_t getLibraryBaseAddress(const std::string& libraryName)
{
    // 打开动态库
    void* handle = dlopen(libraryName.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "Failed to open library: " << dlerror() << std::endl;
        return 0;
    }

    // 获取动态库的链接信息
    struct link_map* map;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
        std::cerr << "Failed to get library info: " << dlerror() << std::endl;
        dlclose(handle);
        return 0;
    }

    // 返回动态库的基地址
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(map->l_addr);
    dlclose(handle);
    return baseAddress;
}
#endif

JNIEXPORT void JNICALL Java_app_LocalJStack_init(JNIEnv *env, jclass cls, jlong threadDumpOffset)
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^---%L---%$] [pid %P thread %t] %v");

    uintptr_t libjvm_base = 
#ifdef OS_WINDOWS
    getLibraryBaseAddress("libjvm.dll");
#elif defined(OS_MACOS)
    getLibraryBaseAddress("libjvm.dylib");
#elif defined(OS_LINUX)
    getLibraryBaseAddress("libjvm.so");
#endif
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
