package com.mxtech.audio

import android.os.AsyncTask
import android.util.Log
//import com.mxplay.logger.ZenLogger
import java.io.File

class AsyncAudioConverter(private val source: String, private val target: String, private val format: String, private val callback: (Boolean)->Unit) : AsyncTask<Void, Void, Boolean>() {
    private var audioConverter: AudioConverter? = null
    override fun doInBackground(vararg params: Void?): Boolean {
        val file = File(target)
        file.parentFile?.mkdirs()
//        val tempPath = "${this.target}.tmp"

        synchronized(this) {
            if (isCancelled)
                return false

//            ZenLogger.ee("AudioConverter") { "start convert: $source, $tempPath, $format" }
            audioConverter = AudioConverter(source, target, format) {
                Log.e("test", "progress on $it")
            }
        }
        val error = audioConverter?.convert()
        synchronized(this) {
            audioConverter?.release()
            audioConverter = null
        }

        if (error == null) {
            return true
//            return File(tempPath).renameTo(file)
        } else {
            Log.e("test", "error $error")
        }

        return false
    }

    fun stop() {
        synchronized(this) {
            cancel(false)
            audioConverter?.stop()
        }
    }

    override fun onPostExecute(result: Boolean) {
//        ZenLogger.ee("AudioConverter") { "convert finish successful ? $result" }

        callback(result)
    }
}