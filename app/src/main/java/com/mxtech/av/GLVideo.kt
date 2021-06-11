package com.mxtech.av

import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.os.SystemClock
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GLVideo (private val path: String): GLSurfaceView.Renderer {
    private var ptr: Long = 0

    init {
        ptr = nativeInit(path)
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
//        gl?.glClearColor(0.0f, 0.0f, 1.0f, 0.0f)
        nativeOnSurfaceCreated(ptr)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        Log.e("test", "onSurfaceChanged $width $height")
        nativeOnSurfaceChanged(ptr, width, height)
        Thread {
            nativeRun(ptr)
        }.start()
    }

    override fun onDrawFrame(gl: GL10?) {
//        gl?.glClear(GLES20.GL_COLOR_BUFFER_BIT)
        nativeOnDrawFrame(ptr, SystemClock.elapsedRealtime());
    }

    fun release() {
        if (ptr == 0L)
            return

        val p = ptr
        ptr = 0
        nativeRelease(p)
    }

    fun onProgress(progress: Int) {

    }

    private external fun nativeRun(p: Long)
    private external fun nativeInit(path: String): Long
    private external fun nativeRelease(p: Long)
    private external fun nativeOnSurfaceCreated(p: Long)
    private external fun nativeOnSurfaceChanged(p: Long, width: Int, height: Int)
    private external fun nativeOnDrawFrame(p: Long, time: Long)

    companion object {
        init {
            nativeInitClass()
        }

        @JvmStatic
        external fun nativeInitClass()
    }
}