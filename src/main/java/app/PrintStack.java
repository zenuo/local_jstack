package app;

import java.io.StringWriter;

class PrintStack {
    public static void main(String[] args) {
        StringWriter writer = new StringWriter();;
        LocalJStack.dumpStack(writer);
        System.out.println(writer);
    }
}
