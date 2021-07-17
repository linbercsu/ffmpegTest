package com.mxtech;

import androidx.annotation.NonNull;

import java.io.PrintStream;

public class NCException extends Exception {

    private final StackTraceElement[] traceElements;
    private final String[] split;

    public NCException(String nativeStack, StackTraceElement[] traceElements) {
        super();
        split = nativeStack.split("\n");
        this.traceElements = traceElements;
    }


    @NonNull
    @Override
    public StackTraceElement[] getStackTrace() {
        return traceElements;
    }

    @SuppressWarnings("SynchronizationOnLocalVariableOrMethodParameter")
    @Override
    public void printStackTrace(@NonNull PrintStream printStream) {
        synchronized (printStream) {
            printStream.println(this);

            for (String str: split) {
                printStream.println("\tat " + str);
            }


            for (StackTraceElement traceElement : traceElements) {
                printStream.println("\tat " + traceElement);
            }
            printStream.println();
        }
    }

}
