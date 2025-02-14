package app;

import java.io.StringWriter;

public class LocalJStackTest {
    public static void main(String[] args) {
        StringWriter writer = new StringWriter();;
        LocalJStack.dumpStack(writer);
        System.err.println(writer);
    }
}
