package com.mxtech.av;

import android.opengl.GLSurfaceView;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.Keep;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class TinyPlayer implements GLSurfaceView.Renderer {
    static {
        nativeInitClass();
    }

    long ref;
    String path;
    boolean stopped;

    int speed = 1;
    int effect = 0;

    private int frameCount;
    private volatile int frameRate;
    private long last;

    private TextView subtitleView;
    public TinyPlayer(String path) {
        this.path = path;
    }

    public void bindGLSurfaceView(GLSurfaceView surfaceView) {
        surfaceView.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override
            public void onViewAttachedToWindow(View view) {

            }

            @Override
            public void onViewDetachedFromWindow(View view) {
                if (stopped) {
                    nativeStop(ref);
                    ref = 0;
                }
            }
        });
        surfaceView.setRenderer(this);
    }

    public void bindSubtitleView(TextView textView) {
        subtitleView = textView;
    }

    public void play() {
        if (ref != 0) {
            throw new RuntimeException("re init.");
        }

        ref = createPlayer(path);
    }

    public void start() {
        start(ref);
    }

    public void pause() {
        pause(ref);
    }

    public void seek() {

    }

    public void forward(long duration) {
        nativeForward(ref, duration);
    }

    public void updateSpeed() {
        if (speed == 1) {
            speed = 2;
        } else {
            speed = 1;
        }

        nativeUpdateSpeed(ref, speed);
    }

    public void updateEffect() {
        if (effect == 0) {
            effect = 1;
        } else if (effect == 1) {
            effect = 2;
        } else {
            effect = 0;
        }
        nativeUpdateEffect(ref, effect);
    }

    public int getFrameRate() {
        return frameRate;
    }

    public void backward(long duration) {
        nativeBackward(ref, duration);
    }

    public void rotate(int rotation) {
        nativeRotate(ref, rotation);
    }

    public void stop() {
//        nativeStop(ref);
//        ref = 0;
        stopped = true;
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
        long now = SystemClock.elapsedRealtime();
        if (now - last > 500) {
            frameRate = (int)(frameCount * 1000 / (now - last));
            last = now;
            frameCount = 0;
        } else {
            frameCount++;
        }
        nativeOnDrawFrame(ref);
    }

    /*
     * called from native
     */
    @Keep
    private void displaySubtitle(String subtitle) {
        Log.e("test", "displaySubtitle: " + subtitle);

        subtitleView.post(new Runnable() {
            @Override
            public void run() {
                subtitleView.setText(subtitle);
            }
        });
    }

    private native long createPlayer(String path);
    private native void start(long ref);
    private native void pause(long ref);
    private native void nativeStop(long ref);
    private native void nativeBackward(long ref, long duration);
    private native void nativeForward(long ref, long duration);
    private native void nativeUpdateSpeed(long ref, int speed);
    private native void nativeUpdateEffect(long ref, int effect);
    private native void nativeRotate(long ref, int rotation);

    private native void nativeOnSurfaceCreated(long ref);
    private native void nativeOnSurfaceChanged(long ref, int w, int h);
    private native void nativeOnDrawFrame(long ref);

    private static native void nativeInitClass();


}
