package com.example.gcsuppress

/**
 * @author Q
 * @date 2023/10/19.
 */
class GCSuppress {
    companion object {
        const val TAG = "MainActivity"
        // Used to load the 'gcsuppress' library on application startup.
        init {
            System.loadLibrary("gcsuppress")
        }
    }

    external fun nativeSuppress(targetHeapUtilization:Float)
}