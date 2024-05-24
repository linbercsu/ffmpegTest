package com.mxtech.av;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class TinyPlayer implements GLSurfaceView.Renderer {
    static {
        nativeInitClass();
    }

    long ref;
    String path;
    public TinyPlayer(String path) {
        this.path = path;
    }

    public void play() {
        if (ref != 0) {
            throw new RuntimeException("re init.");
        }

        ref = createPlayer(path);
    }

    public void start() {

    }

    public void pause() {

    }

    public void seek() {

    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        nativeOnSurfaceCreated(ref);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int w, int h) {
        nativeOnSurfaceChanged(ref, w, h);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        nativeOnDrawFrame(ref);
    }

    private native long createPlayer(String path);
    private native void start(long ref);
    private native void pause(long ref);
    private native void nativeOnSurfaceCreated(long ref);
    private native void nativeOnSurfaceChanged(long ref, int w, int h);
    private native void nativeOnDrawFrame(long ref);

    private static native void nativeInitClass();


}
