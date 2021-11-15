package com.mxtech.av

class MediaConverter2(source: String, target: String, format: String, private val progress: (Int)->Unit) {
    private var ptr: Long
    var stopped: Boolean = false

    init {
        ptr = nativeInit(source, target, format)
    }

    fun convert(start: Long = -1, duration: Long = -1, audioIndex: Int = -1, videoIndex: Int = -1): String? {
        return nativeConvert(ptr, start, duration, audioIndex, videoIndex)
    }

    fun stop() {
        nativeStop(ptr)
    }

    fun release() {
        if (ptr == 0L)
            return

        nativeRelease(ptr)
        ptr = 0
    }

    fun onProgress(progress: Int) {
        this.progress(progress)
    }

    private external fun nativeInit(source: String, target: String, format: String): Long
    private external fun nativeConvert(
        ptr: Long,
        start: Long,
        duration: Long,
        audioIndex: Int,
        videoIndex: Int
    ): String?
    private external fun nativeStop(ptr: Long)
    private external fun nativeRelease(ptr: Long)

    companion object {
        init {
            nativeInitClass()
        }

        @JvmStatic
        private external fun nativeInitClass()

    }

}