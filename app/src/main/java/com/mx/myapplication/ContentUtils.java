package com.mx.myapplication;

import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.text.TextUtils;


import java.io.File;
import java.util.List;

/**
 * Retrieves the file Uri from the uri from different content providers, document providers and file providers
 */

public class ContentUtils {

    static class Files {
        public static boolean isReadableFile(String path) {
            return true;
        }
    }
    private static final String TAG = "ContentUtils";

    private static boolean isFileUri(Uri uri) {
        String scheme = uri.getScheme();
        if (TextUtils.isEmpty(scheme)) {
            return true;
        }

        if (scheme.equals("file") || scheme.equals("FILE")) {
            return true;
        }

        return false;
    }
    public static Uri getFileUriFromContentUri(Context context, final Uri uri) {

        if (uri == null) {
            return null;

        } else if (isFileUri(uri)) {
            /*
            * some application may pass non-latin characters as it is. It can cause issues with Navigator.
            * So, retrieve fully formatted file uri from file object.
            */
            File file = new File(uri.getPath());
            return file.exists() ? Uri.fromFile(file) : uri;

        } else if ("content".equalsIgnoreCase(uri.getScheme())) {

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
                /*
                 * Android versions prior to KITKAT doesn't have document provider.
                 * So, only possible method is to retrieve it from media store
                 */
                return getFileUri(getDataColumn(context, uri, null, null));

            } else if (DocumentsContract.isDocumentUri(context, uri)) {
                /*
                 * Handles different type of Uris from Document Provider.
                 */

                // #debug debug
//@                Log.d(TAG, "type -> DocumentUri; authority -> " + uri.getEncodedAuthority());

                if (isExternalStorageDocument(uri)) {
                    final String docId = DocumentsContract.getDocumentId(uri);
                    final String[] split = docId.split(":");
                    final String type = split[0];

                    if ("primary".equalsIgnoreCase(type)) {
                        if (split.length > 1) {
                            return getFileUri(Environment.getExternalStorageDirectory() + "/" + split[1]);
                        } else {
                            return getFileUri(Environment.getExternalStorageDirectory() + "/");
                        }
                    } else {
                        return getFileUri("/storage" + "/" + docId.replace(":", "/"));
                    }

                } else if (isDownloadsDocument(uri)) {
                    String fileName = getFilePath(context, uri);
                    if (fileName != null) {
                        String filepath = Environment.getExternalStorageDirectory().toString() + "/Download/" + fileName;
                        if (Files.isReadableFile(filepath))
                            return getFileUri(filepath);
                    }

                    String id = DocumentsContract.getDocumentId(uri);
                    if (id.startsWith("raw:")) {
                        id = id.replaceFirst("raw:", "");
                        if (Files.isReadableFile(id))
                            return getFileUri(id);
                    }

                    final Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downloads/public_downloads"), Long.parseLong(id));
                    return getFileUri(getDataColumn(context, contentUri, null, null));

                } else if (isMediaDocument(uri)) {
                    final String docId = DocumentsContract.getDocumentId(uri);
                    final String[] split = docId.split(":");
                    final String type = split[0];

                    Uri contentUri = null;
                    if ("image".equals(type)) {
                        contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                    } else if ("video".equals(type)) {
                        contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                    } else if ("audio".equals(type)) {
                        contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                    }

                    final String selection = MediaStore.MediaColumns._ID + "=?";
                    final String[] selectionArgs = new String[]{ split[1] };
                    return getFileUri(getDataColumn(context, contentUri, selection, selectionArgs));
                }

            } else if (isMediaStoreUri(uri)){
                /*
                * Use existing methods to retrieve the file uri from MediaStore Uri
                */

                // #debug debug
//@                Log.d(TAG, "type -> MediaStoreUri");

                return getFileUri(getDataColumn(context, uri, null, null));

            } else if (isGooglePhotosUri(uri)) {
                // #debug debug
//@                Log.d(TAG, "type -> GooglePhotosUri");

                return getFileUri(uri.getLastPathSegment());

            } else {
                /*
                * Retrieve file uri if it is from file provider.
                * File Provider may have different Uri formats.
                * When used without any options file uri will be directly appended after authority.
                * When a name specified for the path attribute, then the file uri will be appended after name.
                * When a name specified for it may replace the external storage path with the name.
                */
                String path = uri.getPath();
                if (!TextUtils.isEmpty(path)) {
                    List<String> pathSegments = uri.getPathSegments();
                    if (pathSegments.size() > 2) {
                        if ("storage".equalsIgnoreCase(pathSegments.get(0)) && Files.isReadableFile(path)) {

                            // #debug debug
//@                            Log.d(TAG, "type -> FileProviderUri v1");

                            return getFileUri(path);
                        } else if ("storage".equalsIgnoreCase(pathSegments.get(1))) {
                            Uri.Builder builder = new Uri.Builder();
                            for (String segment : pathSegments.subList(1, pathSegments.size()))
                                builder.appendEncodedPath(segment);
                            if (Files.isReadableFile(builder.toString())) {

                                // #debug debug
//@                                Log.d(TAG, "type -> FileProviderUri v2");

                                return getFileUri(builder.build().getPath());
                            }
                        } else {
                            Uri.Builder builder = Uri.parse(Environment.getExternalStorageDirectory().getPath()).buildUpon();
                            for (String segment : pathSegments.subList(1, pathSegments.size()))
                                builder.appendEncodedPath(segment);
                            if (Files.isReadableFile(builder.toString())) {

                                // #debug debug
//@                                Log.d(TAG, "type -> FileProviderUri v3");

                                return getFileUri(builder.build().getPath());
                            }
                        }
                    }
                }

                // #debug debug
//@                Log.d(TAG, "type -> UnknownContentUri");

                // fallback to MediaStore method if none of the above methods work.
                return getFileUri(getDataColumn(context, uri, null, null));
            }
        }

        return null;
    }

    private static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs) {
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { MediaStore.MediaColumns.DATA }, selection, selectionArgs, null);
            if (cursor != null && cursor.moveToFirst()) {
                final int index = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DATA); //make sure that requested column is available
                return cursor.getString(index);
            }
        } catch (Exception ignore) {

        } finally {
            if (cursor != null)
                cursor.close();
        }
        return null;
    }

    private static String getFilePath(Context context, Uri uri) {
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { MediaStore.MediaColumns.DISPLAY_NAME }, null, null,
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                final int index = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DISPLAY_NAME); //make sure that requested column is available
                return cursor.getString(index);
            }
        } catch (Exception ignore) {

        } finally {
            if (cursor != null)
                cursor.close();
        }
        return null;
    }

    private static boolean isExternalStorageDocument(Uri uri) {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    private static boolean isDownloadsDocument(Uri uri) {
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    private static boolean isMediaDocument(Uri uri) {
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    private static boolean isGooglePhotosUri(Uri uri) {
        return "com.google.android.apps.photos.content".equals(uri.getAuthority());
    }

    private static boolean isMediaStoreUri(Uri uri) {
        return MediaStore.AUTHORITY.equals(uri.getHost());
    }

    private static Uri getFileUri(String path) {
        return Files.isReadableFile(path) ? Uri.fromFile(new File(path)) : null;
    }
}
