package com.mxtech;

import android.content.Context;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import okio.BufferedSource;
import okio.Okio;
import okio.Source;

public class NativeCrashCollector {

    public static final String TAG = "mx-nc";

    public interface Callback {
        void onNativeCrash(@NonNull Exception exception);
    }

    private static Callback s_callback = null;
    private static File s_crashDir = null;
    private static Executor s_executor = null;
    private static boolean s_debug;
    public static void init(Context context, Executor executor, Callback callback, boolean debug) {
        if (executor == null) {
            ThreadPoolExecutor threadPoolExecutor = new ThreadPoolExecutor(1, 1,
                    0L, TimeUnit.MILLISECONDS,
                    new LinkedBlockingQueue<Runnable>());
            threadPoolExecutor.setKeepAliveTime(1, TimeUnit.SECONDS);
            threadPoolExecutor.allowCoreThreadTimeOut(true);

            executor = threadPoolExecutor;
        }

        s_debug = debug;
        s_executor = executor;
        s_callback = callback;
        s_crashDir = new File(context.getExternalCacheDir(), "nc");

        s_crashDir.mkdirs();
        nativeInitClass(s_crashDir.getAbsolutePath());
    }

    public static void startReport() {
        s_executor.execute(new Runnable() {
            @Override
            public void run() {
                if (s_debug)
                    Log.w(TAG, "start to report native crash.");
                listNativeCrash();
                listNativeAndJavaCrash();
            }
        });
    }

    private static void listNativeCrash() {
        File[] files = s_crashDir.listFiles();
        if (files == null)
            return;

        for (File file1 : files) {
            String name = file1.getName();
            if (!name.startsWith("nc_") || !name.endsWith(".txt")) {
                continue;
            }

            if (s_debug)
                Log.w(TAG, "find one native crash. " + name);

            try {
                Source source = Okio.source(file1);
                BufferedSource buffer = Okio.buffer(source);
                byte[] bytes = buffer.readByteArray();
                source.close();
                file1.delete();
                String s = new String(bytes);
                NCException ncException = new NCException(s, null);
                callback(ncException);
//                        onNativeCrash(s);
            } catch (Exception e) {
                e.printStackTrace();
            }

        }
    }
    private static void listNativeAndJavaCrash() {
        File dir = new File(s_crashDir, "nc_java");
        File[] files = dir.listFiles();
        if (files == null)
            return;

        for (File file1 : files) {
            String name = file1.getName();
            if (!name.startsWith("nc_") || !name.endsWith(".txt")) {
                continue;
            }

            if (s_debug)
                Log.w(TAG, "find one java&native crash." + name);

            try {
                NCException ncException = NCException.createFromFile(file1);
                if (ncException != null) {
                    s_callback.onNativeCrash(ncException);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            file1.delete();

        }
    }

    @Keep
    private static void onNativeCrash(String log) {
        NCException ncException = new NCException(log, Thread.currentThread().getStackTrace());

        File dir = new File(s_crashDir, "nc_java");
        dir.mkdirs();
        long now = System.currentTimeMillis();
        String fileName = "nc_" + now + ".txt";
        File file = new File(dir, fileName);
        try {
            ncException.writeToFile(file);
        } catch (Exception e) {
            e.printStackTrace();
        }
//        s_callback.onNativeCrash(ncException);
    }

    private static void callback(NCException e) {
        s_callback.onNativeCrash(e);
    }


    private static native void nativeInitClass(String dir);
}
