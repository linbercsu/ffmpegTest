package com.mx.myapplication

import android.app.Activity
import android.content.Intent
import android.os.AsyncTask
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    private var audioConverter: Converter? = null;

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        val textView = findViewById<TextView>(R.id.sample_text)
//        textView.text = stringFromJNI()
//        Log.e("test", "test: max: ${max()}")


        audioConverter = Converter()
        audioConverter?.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)

        textView.setOnClickListener {
            audioConverter?.stop()
        }
    }

    override fun onStop() {
        audioConverter?.stop()
        super.onStop()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun max(): Int
//    external fun convert(source: String, target: String, format: String);

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("mp3lame")
            System.loadLibrary("avcodec")
            System.loadLibrary("avdevice")
            System.loadLibrary("avfilter")
            System.loadLibrary("avformat")
            System.loadLibrary("avutil")
            System.loadLibrary("postproc")
            System.loadLibrary("swresample")
            System.loadLibrary("swscale")
            System.loadLibrary("native-lib")
        }

        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, MainActivity::class.java));
        }
    }

    class Converter : AsyncTask<Void, Void, Boolean>() {
        private var audioConverter: AudioConverter? = null
        override fun doInBackground(vararg params: Void?): Boolean {
            synchronized(this) {
                if (isCancelled)
                    return false

                audioConverter = AudioConverter("/sdcard/test1/big.mp4", "/sdcard/test1/big.mp3", "mp3")
            }
            audioConverter?.convert()
            synchronized(this) {
                audioConverter?.release()
                audioConverter = null
            }
            return true
        }

        fun stop() {
            synchronized(this) {
                cancel(false)
                audioConverter?.stop()
            }
        }

        override fun onCancelled() {
            super.onCancelled()
            Log.e("test", "convert cancel")
        }

        override fun onPostExecute(result: Boolean?) {

            Log.e("test", "convert finish")
        }
    }
}