package com.mx.myapplication

class AudioConverter(source: String, target: String, format: String) {
    private var ptr: Long
    var stopped: Boolean = false

    init {
        ptr = nativeInit(source, target, format)
    }

    fun convert() {
        nativeConvert(ptr)
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

    private external fun nativeInit(source: String, target: String, format: String): Long
    private external fun nativeConvert(ptr: Long)
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