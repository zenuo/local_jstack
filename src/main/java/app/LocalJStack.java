package app;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Writer;

import app.OsUtils.OSType;

/**
 * 本地JStack
 */
public class LocalJStack {

    private static void loadLib() {
        // 尝试从系统库路径加载
        try {
            System.loadLibrary("localjstack");
            // 加载成功
            return;
        } catch (Throwable e) {
            // ignore
        }

        // 尝试从 JAR 中提取 .so 文件到临时目录
        try {
            String dynamicLibraryName = getDynamicLibraryName();

            InputStream in = LocalJStack.class.getClassLoader().getResourceAsStream(dynamicLibraryName);
            File tempFile = File.createTempFile("liblocaljstack", ".so");
            try (OutputStream out = new FileOutputStream(tempFile)) {
                byte[] buffer = new byte[1024];
                int bytesRead;
                while ((bytesRead = in.read(buffer)) != -1) {
                    out.write(buffer, 0, bytesRead);
                }
            } finally {
                in.close();    
            }
            
            // 加载 .so 文件
            System.load(tempFile.getAbsolutePath());
            return;
        } catch (Throwable e) {
            // ignore
        }

        throw new RuntimeException("Failed to load liblocaljstack.so");
    }

    static {
        loadLib();

        String threadDumpOffsetInHex = System.getProperty("localjstack.threadDumpOffset");

        long threadDumpOffset = Long.parseLong(threadDumpOffsetInHex, 16);

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

    private static String getDynamicLibraryName() {
        OSType osType = OsUtils.getOperatingSystemType();
        switch (osType) {
            case MacOS:
                return "liblocaljstack.dylib";
        
            case Linux:
                return "liblocaljstack.so.0.1";
            
            case Windows:
                return "localjstack.dll";
        
            default:
                throw new RuntimeException("Unsupported OS: " + System.getProperty("os.name"));
        }
    }
}