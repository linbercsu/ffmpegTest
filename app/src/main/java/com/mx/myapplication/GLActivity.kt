package com.mx.myapplication

import android.app.Activity
import android.content.Intent
import android.opengl.GLES20.GL_COLOR_BUFFER_BIT
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import com.mxtech.av.GLVideo
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GLActivity : AppCompatActivity() {
    private lateinit var glSurfaceView: GLSurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_gl)
        val root = findViewById<FrameLayout>(R.id.gl_root)
        glSurfaceView = GLSurfaceView(root.context)

        glSurfaceView.setEGLContextClientVersion(2)
        glSurfaceView.setRenderer(GLVideo("/sdcard/test1/big.mp4"))

        root.addView(glSurfaceView, -1, -1)
    }

    override fun onResume() {
        super.onResume()
        glSurfaceView.onResume()
    }

    override fun onPause() {
        glSurfaceView.onPause()
        super.onPause()
    }

    class GLRenderer : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            gl?.glClearColor(0.0f, 0.0f, 1.0f, 0.0f)
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {

        }

        override fun onDrawFrame(gl: GL10?) {
            gl?.glClear(GL_COLOR_BUFFER_BIT)
        }

    }

    companion object {

        fun start(activity: Activity) {
            activity.startActivity(Intent(activity, GLActivity::class.java));
        }
    }


}