package com.mx.myapplication

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.opengl.GLSurfaceView
import android.os.AsyncTask
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.FrameLayout
import android.widget.TextView
import com.mxtech.NativeCrashCollector
import com.mxtech.av.AsyncAudioConverter
import com.mxtech.av.AsyncMediaConverter
import com.mxtech.av.AsyncMediaConverter2
import com.mxtech.av.GLVideo
import com.mxtech.av.TinyPlayer
import java.io.File

class MainActivity : AppCompatActivity() {
    private lateinit var glSurfaceView: GLSurfaceView

    private lateinit var audioConverter: AsyncMediaConverter2
    private lateinit var converter: AsyncAudioConverter
    private lateinit var player: TinyPlayer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val intent = intent

        val uri = intent.data
        val contentUri = ContentUtils.getFileUriFromContentUri(this, uri)
        val fileUri = if (contentUri != null) {
            contentUri
        } else {
            Uri.fromFile(File("/sdcard/Download/joke.mp4"))
        }


        setContentView(R.layout.activity_main)
        val root = findViewById<FrameLayout>(R.id.gl_root)
        glSurfaceView = GLSurfaceView(root.context)

        glSurfaceView.setEGLContextClientVersion(2)


        root.addView(glSurfaceView, -1, -1)

//        player = TinyPlayer("/sdcard/Download/joke.mp4")
        player = TinyPlayer(fileUri.toString())
        player.play()

        player.bindGLSurfaceView(glSurfaceView);
//        glSurfaceView.setRenderer(player)

        findViewById<View>(R.id.start).setOnClickListener {
            player.start()
        }

        findViewById<View>(R.id.pause).setOnClickListener {
            player.pause()
        }

        findViewById<View>(R.id.back).setOnClickListener {
            player.backward(10 * 1000 * 1000)
        }
        findViewById<View>(R.id.forward).setOnClickListener {
            player.forward(10 * 1000 * 1000)
        }

        findViewById<View>(R.id.speed).setOnClickListener {
            player.updateSpeed()
        }
        findViewById<View>(R.id.gray).setOnClickListener {
            player.updateEffect()
        }
        // Example of a call to a native method
//        val textView = findViewById<TextView>(R.id.sample_text)
        /*
//        textView.text = stringFromJNI()
//        Log.e("test", "test: max: ${max()}")

            audioConverter = AsyncMediaConverter2("/sdcard/test1/big.mp4", "/sdcard/test1/dash/test1.mp4", "mp4", 40000000, 1000000 * 60) {
//            audioConverter = AsyncMediaConverter("/sdcard/1/binkvideo_binkaudio_rdft.bik", "/sdcard/1/test/test.mpd", "dash") {


            }
//
        audioConverter.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
//
//            converter = AsyncAudioConverter("/sdcard/test1/10.wmv", "/sdcard/test1/wmv.mp3", "mp3") {
//
//
//            }
//
//        converter.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)

        textView.setOnClickListener {
            audioConverter.stop()
        }

         */

//        textView.setOnClickListener {
//
//        }
    }

    override fun onStop() {
//        audioConverter.stop()
        super.onStop()
    }

    override fun onBackPressed() {
        player.stop();
        super.onBackPressed()
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


        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, MainActivity::class.java));
        }
    }
}