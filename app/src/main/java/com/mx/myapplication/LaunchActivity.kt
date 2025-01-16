package com.mx.myapplication

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat

class LaunchActivity : AppCompatActivity() {

    private val handler = Handler(Looper.getMainLooper())

    companion object {
        const val PERMISSION_CODE = 1;
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

//        checkPermissions()
    }

    fun requestPermission() {
        val permissions = arrayOf(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
        )

        ActivityCompat.requestPermissions(this, permissions, PERMISSION_CODE);
    }

    override fun onResume() {
        super.onResume()

        checkPermissions()
    }

    fun checkPermissions() {

        if (AllFileManagerPermissionUtil.isAllFileManagerPermissionGranted()) {
            EntryActivity.start(this)
        } else {
            AllFileManagerPermissionUtil.requestAllFilePermission(this, AllFileManagerPermissionUtil.REQUEST_MANAGE_ALL_FILES_ACCESS_PERMISSION)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        if (requestCode != PERMISSION_CODE) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults)
            return
        }

        handler.post() {
            checkPermissions()
        }
    }
}