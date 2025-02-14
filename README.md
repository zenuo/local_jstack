# local_jstack
Thread dump like jstack in local JVM process (not interprocess) by dirty JNI way

## 限制 limitation

仅支持linux

## 上手
## get start

1.  首先找到JRE环境的`libjvm.so`中的[thread_dump(AttachOperation*, outputStream*)](https://github.com/openjdk/jdk/blob/742e735d7f6c4ee9ca5a4d290c59d7d6ec1f7635/src/hotspot/share/services/attachListener.cpp#L209)函数的偏移量

    以21.0.5-ms为例，为`4892880`：

    ```
    $ nm -t d -C /usr/local/sdkman/candidates/java/21.0.5-ms/lib/server/libjvm.so |grep -F 'thread_dump(AttachOperation*, outputStream*)'
    0000000004892880 t thread_dump(AttachOperation*, outputStream*)
    ```

2. 在JVM参数中指定`-Dlocaljstack.threadDumpOffset=4892880`

3. pom.xml中添加本项目:
    ```xml
    <dependency>
        <groupId>app</groupId>
        <artifactId>localjstack</artifactId>
        <version>0.0.1</version>
    </dependency>
    ```

4. 使用java.io.Writer来承接线程栈
    ```java
    java.io.StringWriter writer = new java.io.StringWriter();;
    app.LocalJStack.dumpStack(writer);
    System.err.println(writer.toString());
    ```
