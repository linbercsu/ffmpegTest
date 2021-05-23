package com.mx.myapplication

import android.app.Activity
import android.content.Intent
import android.os.AsyncTask
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import com.mxtech.audio.AsyncAudioConverter

class MainActivity : AppCompatActivity() {

    private lateinit var audioConverter: AsyncAudioConverter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        val textView = findViewById<TextView>(R.id.sample_text)
//        textView.text = stringFromJNI()
//        Log.e("test", "test: max: ${max()}")

            audioConverter = AsyncAudioConverter("/sdcard/test1/big.mp4", "/sdcard/test1/big.mp3", "mp3") {


            }

        audioConverter.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)

        textView.setOnClickListener {
            audioConverter.stop()
        }
    }

    override fun onStop() {
        audioConverter.stop()
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
            System.loadLibrary("native-lib")
        }

        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, MainActivity::class.java));
        }
    }
}