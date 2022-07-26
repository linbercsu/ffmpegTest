package com.mxtech.av

import android.os.AsyncTask
import android.util.Log
//import com.mxplay.logger.ZenLogger
import java.io.File
import java.util.concurrent.Executor

class AsyncMediaConverter2(private val source: String, private val target: String, private val format: String, private val start: Long = -1, private val duration: Long = -1, private val audioIndex: Int = -1, private val videoIndex: Int = -1, private val callback: (Boolean)->Unit) : AsyncTask<Void, Void, Boolean>() {
    private var converter: MediaConverter2? = null
    private val dash: Boolean = ("dash" == format)

    override fun doInBackground(vararg params: Void?): Boolean {
        val file = File(target)
        file.parentFile?.mkdirs()

        val tempPath = "${this.target}.tmp"

        synchronized(this) {
            if (isCancelled)
                return false

//            ZenLogger.ee("AudioConverter") { "start convert: $source, $tempPath, $format" }
            val tar: String = if (dash) {
                target
            } else {
                tempPath
            }

            File(tar).parentFile.mkdirs()

            converter = MediaConverter2(source, tar, format) {
                Log.e("MediaConverter2", "progress on $it")
            }
        }
        val error = converter?.convert(start, duration)
        synchronized(this) {
            converter?.release()
            converter = null
        }

        if (error == null) {
            if (dash) {
                return true
            }
            return File(tempPath).renameTo(file)
        } else {
            Log.e("test", "error $error")
        }

        return false
    }

    fun start(executor: Executor) {
        executeOnExecutor(executor)
    }

    fun stop() {
        synchronized(this) {
            cancel(false)
            converter?.stop()
        }
    }

    override fun onPostExecute(result: Boolean) {
//        ZenLogger.ee("AudioConverter") { "convert finish successful ? $result" }

        callback(result)
    }
}