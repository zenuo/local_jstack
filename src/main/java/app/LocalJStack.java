package app;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Writer;

/**
 * 本地JStack
 */
public class LocalJStack {

    private static void loadLib() {
        try {
            // 尝试从系统库路径加载
            try {
                System.loadLibrary("localjstack");
                // 加载成功
                return;
            } catch (Exception e) {
                // ignore
            }

            // 尝试从 JAR 中提取 .so 文件到临时目录
            InputStream in = LocalJStack.class.getResourceAsStream("liblocaljstack.so.0.1");
            File tempFile = File.createTempFile("liblocaljstack", ".so");
            OutputStream out = new FileOutputStream(tempFile);

            byte[] buffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = in.read(buffer)) != -1) {
                out.write(buffer, 0, bytesRead);
            }
            out.close();
            in.close();

            // 加载 .so 文件
            System.load(tempFile.getAbsolutePath());
            return;
        } catch (Exception e) {
            // ignore
        }

        throw new RuntimeException("Failed to load liblocaljstack.so");
    }

    static {
        loadLib();

        long threadDumpOffset = Long.valueOf(System.getProperty("localjstack.threadDumpOffset", "0"));

        init(threadDumpOffset);
    }

    /**
     * 初始化
     * 
     * @param threadDumpOffset thread_dump(AttachOperation*, outputStream*)偏移量
     * 
     * @see <a href=
     *      "https://github.com/openjdk/jdk/blob/742e735d7f6c4ee9ca5a4d290c59d7d6ec1f7635/src/hotspot/share/services/attachListener.cpp#L209"/>
     */
    private static native void init(long threadDumpOffset);

    /**
     * 打印当前JVM的线程栈
     * 
     * @param writer 输出流
     * @return 执行是否成功，0表示成功
     */
    public static synchronized native int dumpStack(Writer writer);
}