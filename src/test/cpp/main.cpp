#include <stdio.h>
#include <jni.h>

#define VM_ARG_nOptions 4

int main(int argc, char *argv[])
{
    printf("main\n");
    JavaVMInitArgs vm_args;
    JavaVMOption options[VM_ARG_nOptions];

    options[0].optionString = "-Dlocaljstack.threadDumpOffset=116b50";
    options[1].optionString = "-Djava.library.path=C:\\download\\local_jstack\\build\\x86_windows_msvc2022_pe_32bit-Debug\\src\\main\\cpp";  /* set native library path */
    options[2].optionString = "-Djava.class.path=C:\\download\\localjstack-0.0.1.jar";
    options[3].optionString = "-verbose:jni";                   /* print JNI-related messages */

    vm_args.version = JNI_VERSION_1_8;
    vm_args.options = options;
    vm_args.nOptions = VM_ARG_nOptions;
    vm_args.ignoreUnrecognized = 0;

    JavaVM *vm;       /* denotes a Java VM */
    JNIEnv *env;

    jint res = JNI_CreateJavaVM(&vm, (void **)&env, &vm_args);

    if (res != JNI_OK) {
        printf("Failed to create Java VMn");
        return 1;
    }

    jclass          cls;
    jmethodID       mid;

    cls = (*env).FindClass("app/PrintStack");
    if (cls == NULL) {
        printf("Failed to find app.PrintStack class\n");
        return 1;
    }

    mid = (*env).GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
    if (mid == NULL) {
        printf("Failed to find main function\n");
        return 1;
    }

    jstring jstr      = (*env).NewStringUTF("");
    jobjectArray main_args = (*env).NewObjectArray(1, (*env).FindClass("java/lang/String"), jstr);
    (*env).CallStaticVoidMethod(cls, mid, main_args);

    return 0;
}
