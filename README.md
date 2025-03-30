# local_jstack

Thread dump like jstack in local JVM process (not interprocess like jstack in jdk) by dirty JNI way. 

在本地 JVM 进程（而不是jdk中jstack的进程间）中通过粗糙的 JNI 方式进行类似 jstack 的线程转储

## 支持的平台 Supported Platform

- linux x64
- macos x64
- windows x64

## 上手 Get Started

1.  首先找到JRE环境的`libjvm.so`中的[thread_dump(AttachOperation*, outputStream*)](https://github.com/openjdk/jdk/blob/742e735d7f6c4ee9ca5a4d290c59d7d6ec1f7635/src/hotspot/share/services/attachListener.cpp#L209)函数的偏移量

    - unix平台，以21.0.5-ms为例，为`4892880`：

        ```cmd
        $ nm -t d -C /usr/local/sdkman/candidates/java/21.0.5-ms/lib/server/libjvm.so |grep -F 'thread_dump(AttachOperation*, outputStream*)'
        0000000004892880 t thread_dump(AttachOperation*, outputStream*)
        ```

    - Windows平台，则需要借助map文件来获取函数偏移量，例如[microsoft-jdk-21.0.3-windows-x64.zip](https://aka.ms/download-jdk/microsoft-jdk-21.0.3-windows-x64.zip)，需要下载微软提供的调试信息[microsoft-jdk-debugsymbols-21.0.3-windows-x64.zip](https://aka.ms/download-jdk/microsoft-jdk-debugsymbols-21.0.3-windows-x64.zip)，然后解压调试信息

        首先是获取`ImageBase`，如下命令的结果第三列`0000000180000000`就是`ImageBase`的Rva+Base 16进制形式：

        ```
        findstr "__ImageBase" C:\jdk-21.0.3+9-debug-symbols\bin\server\jvm.dll.map
         0000:00000000       __ImageBase                0000000180000000     <linker-defined>
        ```

        再是获取`thread_dump(AttachOperation*, outputStream*)`方法在dll中的地址，如下命令的结果第三列`00000001801315e0`就是`thread_dump`函数Rva+Base16进制形式：

        ```cmd
        findstr "?thread_dump@@YAJPEAVAttachOperation@@PEAVoutputStream@@@Z" C:\jdk-21.0.3+9-debug-symbols\bin\server\jvm.dll.map | findstr /V unwind
         0001:001305e0       ?thread_dump@@YAJPEAVAttachOperation@@PEAVoutputStream@@@Z 00000001801315e0 f   attachListener.obj
        ```

        然后两者做减法：`00000001801315e0 - 0000000180000000`的结果`1315e0`就是`thread_dump`函数在`jvm.dll`中的偏移量

2. 使用java.io.Writer来承接线程栈
    ```java
    import java.io.StringWriter;

    public class MyApp {
        public static void main(String[] args) {
            StringWriter writer = new StringWriter();;
            LocalJStack.dumpStack(writer);
            System.out.println(writer);
        }
    }
    ```

3. 编译、运行
    ```
    $ javac -cp localjstack-0.0.1.jar MyApp.java
    $ /usr/local/sdkman/candidates/java/21.0.5-ms/bin/java -Dlocaljstack.threadDumpOffset=4892880 -cp localjstack-0.0.1.jar:. MyApp
    ```

    打印到标准输出:

    ```
    2025-02-19 14:58:09
    Full thread dump OpenJDK 64-Bit Server VM (21.0.5+11-LTS mixed mode, sharing):

    Threads class SMR info:
    _java_thread_list=0x000077b8f415e3b0, length=10, elements={
    ...
    ```
    
    如果使用Maven，在pom.xml中添加本项目:
    ```xml
    <dependency>
        <groupId>app</groupId>
        <artifactId>localjstack</artifactId>
        <version>0.0.1</version>
    </dependency>
    ```

## 编译源代码 Build From Source

```
$ cmake --build ./build --config Debug --target LibLocalJStack -j 4 --
$ mvn package
```

产出jar包为`target/localjstack-0.0.1.jar`
