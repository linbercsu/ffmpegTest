package com.mxtech;

import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import java.io.IOException;

public class NativeCrashCollector {
//    static {
//        System.loadLibrary("mx-nc-collector");
//    }

    public interface Callback {
        void onNativeCrash(@NonNull Exception exception);
    }

    private static Callback s_callback = null;
    public static void init(Callback callback) {
        s_callback = callback;
        nativeInitClass();
    }

    @Keep
    private static void onNativeCrash(String log) {
        NCException ncException = new NCException(log, Thread.currentThread().getStackTrace());
        s_callback.onNativeCrash(ncException);
    }

    private static native void nativeInitClass();
}
