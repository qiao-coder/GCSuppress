package com.example.gcsuppress

import android.annotation.SuppressLint
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import com.example.gcsuppress.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    @SuppressLint("SoonBlockedPrivateApi")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)


        val start = System.nanoTime()
        try {
            val vmRuntimeClass = Class.forName("dalvik.system.VMRuntime")
            val getMethod = vmRuntimeClass.getDeclaredMethod("getTargetHeapUtilization")
            val getRuntime = vmRuntimeClass.getDeclaredMethod("getRuntime")
            val runtime = getRuntime.invoke(null)
            val targetHeapUtilization = getMethod.invoke(runtime) as Float
            Log.d(TAG,"targetHeapUtilization : $targetHeapUtilization")
            GCSuppress().nativeSuppress(targetHeapUtilization)
        } catch (e: Exception) {
            e.printStackTrace()
        }
        Log.e(TAG,"cost : ${(System.nanoTime() - start) / 1000} μs")
    }

    companion object {
        const val TAG = "MainActivity"
    }
}