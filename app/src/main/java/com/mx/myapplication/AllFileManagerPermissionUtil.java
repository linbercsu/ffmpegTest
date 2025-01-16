package com.mx.myapplication;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;


public class AllFileManagerPermissionUtil {

    public static final int REQUEST_MANAGE_ALL_FILES_ACCESS_PERMISSION = 0xff - 100;

    public static boolean allFileManagerPermissionMode() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.R;
    }

    @SuppressLint("NewApi")
    public static boolean isAllFileManagerPermissionGranted(){
        return !allFileManagerPermissionMode() || Environment.isExternalStorageManager();
    }

    public static void requestAllFilePermission(Activity activity, int requestCode) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
            return;

        try {
            Uri uri = Uri.parse("package:" + activity.getPackageName());
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri);
            activity.startActivityForResult(intent, requestCode);
        } catch (ActivityNotFoundException ignore) {
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                activity.startActivityForResult(intent, requestCode);
            } catch ( ActivityNotFoundException e ) {
                try {
                    Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    Uri uri = Uri.fromParts("package", activity.getPackageName(), null);
                    intent.setData(uri);
                    activity.startActivityForResult(intent, requestCode);
                } catch (Throwable th) {}
            }
        }

    }
}
