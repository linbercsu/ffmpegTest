package com.mxtech;

import android.text.TextUtils;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.LinkedList;

import okio.BufferedSink;
import okio.BufferedSource;
import okio.Okio;
import okio.Sink;
import okio.Source;

public class NCException extends Exception {
    private static final String SEP = "::";
    private static final String NULL_STR = "__0_null__";

    private StackTraceElement[] traceElements;
    private String msg;

    public NCException() {
        super();
    }

    @Override
    public String getMessage() {
        return msg;
    }

    public NCException(String nativeStack, StackTraceElement[] traceElements) {
        super();
        LinkedList<StackTraceElement> list = new LinkedList<>();
        String[] split = nativeStack.split("\n");
        for (int i = 0; i < split.length; i++) {
            if (i == 0) {
                msg = split[i];
            } else {
                String str = split[i];
                String[] splitLine = str.split(":");
                if (splitLine.length == 3) {
                    String lib = splitLine[0];
                    String function = splitLine[1];
                    String address = splitLine[2];
                    StackTraceElement stackTraceElement = new StackTraceElement(lib, function + ':' + address, lib, -2);
                    list.add(stackTraceElement);
                }
            }

        }

        //filter dirty native stack trace
        if (list.size() > 4) {
            int index = -1;
            for (int i = 0; i < list.size(); i++) {
                StackTraceElement stackTraceElement = list.get(i);
                if (stackTraceElement.getMethodName().startsWith("__kernel_rt_sigreturn")) {
                    index = i;
                    break;
                }
            }

            if (index != -1) {
                if (TextUtils.equals(list.get(0).getClassName(), list.get(index + 1).getClassName())
                        && TextUtils.equals(list.get(0).getMethodName(), list.get(index + 1).getMethodName())
                ) {
                    for (int i = 0; i <= index; i++) {
                        list.remove(0);
                    }
                }
            }
        }


        if (traceElements != null) {
            //filter dirty java stack trace
            int index = -1;
            for (int i = 0; i < traceElements.length; i++) {
                StackTraceElement traceElement = traceElements[i];
                if (index == -1) {
                    if (TextUtils.equals(traceElement.getClassName(), "com.mxtech.NativeCrashCollector")
                            && TextUtils.equals(traceElement.getMethodName(), "onNativeCrash")){
                        index = i;
                    }
                } else {
                    list.add(traceElement);
                }
            }

//            list.addAll(Arrays.asList(traceElements));
        }


//        list.add(0, fakeTrace());
        this.traceElements = list.toArray(new StackTraceElement[0]);
    }


    public String getMsg() {
        return msg;
    }

    @NonNull
    @Override
    public StackTraceElement[] getStackTrace() {
        return traceElements;
    }

    @Override
    public void printStackTrace() {
        super.printStackTrace();
    }

    @Override
    public void printStackTrace(PrintWriter printStream) {
        synchronized (printStream) {
            printStream.println(this);

            for (StackTraceElement traceElement : traceElements) {
                printStream.println("\tat " + traceElement);
            }
            printStream.println();
        }
    }

    @SuppressWarnings("SynchronizationOnLocalVariableOrMethodParameter")
    @Override
    public void printStackTrace(@NonNull PrintStream printStream) {
        synchronized (printStream) {
            printStream.println(this);

            for (StackTraceElement traceElement : traceElements) {
                printStream.println("\tat " + traceElement);
            }
            printStream.println();
        }
    }

    private static StackTraceElement fakeTrace() {
        return new StackTraceElement(NCException.class.getName(), "fakeMethod", "NCException.java", -2);
    }

    public static NCException createFromFile(File file) throws IOException {
        String content;
        Source source = Okio.source(file);
        try {
            BufferedSource buffer = Okio.buffer(source);
            byte[] bytes = buffer.readByteArray();
            content = new String(bytes);
        } finally {
            source.close();
        }

        String[] split = content.split("\n");
        if (split.length < 4)
            return null;

        String msg = split[0];
        LinkedList<StackTraceElement> list = new LinkedList<>();
        for (int i = 1; i < split.length; i++) {
            String str = split[i];
            if (TextUtils.isEmpty(str))
                continue;

            String[] split1 = str.split(SEP);
            if (split1.length != 4) {
                return null;
            }

            String className = split1[0];
            String methodName = split1[1];
            String fileName = split1[2];
            int lineNumber = Integer.parseInt(split1[3]);

            if (TextUtils.isEmpty(className) || TextUtils.isEmpty(methodName))
                return null;

            if (TextUtils.equals(fileName, NULL_STR)) {
                fileName = null;
            }


            list.add(new StackTraceElement(className, methodName, fileName, lineNumber));
        }

        NCException ncException = new NCException();
        ncException.msg = msg;
        ncException.traceElements = list.toArray(new StackTraceElement[0]);
        return ncException;
    }

    public void writeToFile(File file) throws FileNotFoundException {
        NCException exception = this;
        Sink sink = Okio.sink(file);
        try {
            BufferedSink buffer = Okio.buffer(sink);
            String msg = exception.getMsg();
            buffer.writeUtf8(msg);
            buffer.writeByte('\n');

            StackTraceElement[] stackTrace = exception.getStackTrace();
            for (StackTraceElement stackTraceElement : stackTrace) {
                writeStack(buffer, stackTraceElement);
                buffer.writeByte('\n');
            }

            buffer.flush();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                sink.close();
            } catch (Exception ignore) {

            }
        }
    }

    private static void writeStack(BufferedSink buffer, StackTraceElement stackTraceElement) throws IOException {

        String className = stackTraceElement.getClassName();
        String methodName = stackTraceElement.getMethodName();
        int lineNumber = stackTraceElement.getLineNumber();
        String fileName = stackTraceElement.getFileName();

        if (TextUtils.isEmpty(fileName)) {
            fileName = NULL_STR;
        }

        buffer.writeUtf8(className);
        buffer.writeUtf8(SEP);
        buffer.writeUtf8(methodName);
        buffer.writeUtf8(SEP);
        buffer.writeUtf8(fileName);
        buffer.writeUtf8(SEP);
        buffer.writeUtf8(String.valueOf(lineNumber));
    }

}
