# local_jstack

在本地 JVM 进程（非进程间）中通过臊气的 JNI 方式进行类似 jstack 的线程转储

Thread dump like jstack in local JVM process (not interprocess) by dirty JNI way

## 支持的平台 Supported Platform

- linux-x86_64
- macos-x86_64

## 上手 Get Started

1.  首先找到JRE环境的`libjvm.so`中的[thread_dump(AttachOperation*, outputStream*)](https://github.com/openjdk/jdk/blob/742e735d7f6c4ee9ca5a4d290c59d7d6ec1f7635/src/hotspot/share/services/attachListener.cpp#L209)函数的偏移量

    以21.0.5-ms为例，为`4892880`：

    ```
    $ nm -t d -C /usr/local/sdkman/candidates/java/21.0.5-ms/lib/server/libjvm.so |grep -F 'thread_dump(AttachOperation*, outputStream*)'
    0000000004892880 t thread_dump(AttachOperation*, outputStream*)
    ```

3. 使用java.io.Writer来承接线程栈
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
