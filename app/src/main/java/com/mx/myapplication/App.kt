package com.mx.myapplication

import android.app.Application
import android.util.Log
import com.mxtech.NativeCrashCollector

class App : Application() {

    override fun onCreate() {
        super.onCreate()

        val callback = NativeCrashCollector.Callback { log ->
            Log.e(
                "test",
                "crash", log
            )
        }

        Log.w("test", "onCreate")

        NativeCrashCollector.init(callback)
    }
}