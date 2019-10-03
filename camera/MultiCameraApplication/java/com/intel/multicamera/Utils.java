/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.multicamera;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.*;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.CamcorderProfile;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Log;
import android.util.Size;
import android.util.SparseIntArray;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class Utils {
    private static final String TAG = "Utils";

    public static final int MEDIA_TYPE_IMAGE = 0;
    public static final int MEDIA_TYPE_VIDEO = 1;

    public static final String IMAGE_FILE_NAME_FORMAT = "'IMG'_yyyyMMdd_HHmmss";
    public static final String VIDEO_FILE_NAME_FORMAT = "'VID'_yyyyMMdd_HHmmss";

    /** See android.hardware.Camera.ACTION_NEW_PICTURE. */
    public static final String ACTION_NEW_PICTURE = "android.hardware.action.NEW_PICTURE";
    /** See android.hardware.Camera.ACTION_NEW_VIDEO. */
    public static final String ACTION_NEW_VIDEO = "android.hardware.action.NEW_VIDEO";

    private static final String VIDEO_BASE_URI = "content://media/external/video/media";

    @SuppressLint("SimpleDateFormat")
    public static File createOutputmediaStorageDir() {
        // To be safe, you should check that the SDCard is mounted
        // using Environment.getExternalStorageState() before doing this.

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            Log.e(TAG, "getExternalStorageState  failed");
            return null;
        }

        File mediaStorageDir =
                new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM),
                         "MultiCamera");
        // This location works best if you want the created images to be shared
        // between applications and persist after your app has been uninstalled.

        // Create the storage directory if it does not exist
        if (!mediaStorageDir.exists()) {
            if (!mediaStorageDir.mkdirs()) {
                Log.e(TAG, "Failed to create directory for DCIM/MultiCamera");
                return null;
            }
        }

        return mediaStorageDir;
    }

    public static void broadcastNewPicture(Context context, ContentValues values) {
        Uri uri = null;
        ContentResolver resolver = context.getContentResolver();
        try {
            uri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                                  new ContentValues(values));
        } catch (Throwable th) {
            // This can happen when the external volume is already mounted, but
            // MediaScanner has not notify MediaProvider to add that volume.
            // The picture is still safe and MediaScanner will find it and
            // insert it into MediaProvider. The only problem is that the user
            // cannot click the thumbnail to review the picture.
            Log.e(TAG, "Failed to write MediaStore" + th);
        } finally {
            Log.v(TAG, "Current Picture URI: " + uri);
        }

        context.sendBroadcast(new Intent(ACTION_NEW_PICTURE, uri));
    }

    public static void broadcastNewVideo(Context context, ContentValues values) {
        Uri uri = null;
        ContentResolver resolver = context.getContentResolver();
        try {
            Uri videoTable = Uri.parse(VIDEO_BASE_URI);
            uri = resolver.insert(videoTable, new ContentValues(values));
        } catch (Exception e) {
            // We failed to insert into the database. This can happen if
            // the SD card is unmounted.
            Log.e(TAG, "failed to add video to media store", e);
            uri = null;
        } finally {
            Log.v(TAG, "Current video URI: " + uri);
        }

        context.sendBroadcast(new Intent(ACTION_NEW_VIDEO, uri));
    }

    public static String getFileNameFromUri(Uri uri) {
        String result = null;
        result = uri.getPath();
        int cut = result.lastIndexOf('/');
        if (cut != -1) {
            result = result.substring(cut + 1);
        }
        return result;
    }

    public static String[] generateFileDetails(int type) {
        File mediaStorageDir = createOutputmediaStorageDir();
        if (mediaStorageDir == null) {
            Log.e(TAG, "createOutputmediaStorageDir failed");
            return null;
        }

        long dateTaken = System.currentTimeMillis();
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat;
        String fileDetails[] = new String[5];
        if (type == MEDIA_TYPE_IMAGE) {
            dateFormat = new SimpleDateFormat(IMAGE_FILE_NAME_FORMAT);
            fileDetails[0] = dateFormat.format(date);
            fileDetails[1] = fileDetails[0] + ".jpg";
            fileDetails[2] = "image/jpeg";
        } else if (type == MEDIA_TYPE_VIDEO) {
            dateFormat = new SimpleDateFormat(VIDEO_FILE_NAME_FORMAT);
            fileDetails[0] = dateFormat.format(date);
            fileDetails[1] = fileDetails[0] + ".mp4";
            fileDetails[2] = "video/mp4";
        } else {
            Log.e(TAG, "Invalid Media Type: " + type);
            return null;
        }

        fileDetails[3] = mediaStorageDir.getPath() + '/' + fileDetails[1];
        fileDetails[4] = Long.toString(dateTaken);
        Log.v(TAG, "Generated filename: " + fileDetails[3]);
        return fileDetails;
    }

    public static ContentValues getContentValues(int type, String fileDetails[], int width,
                                                 int height) {
        if (fileDetails.length < 5) {
            Log.e(TAG, "Invalid file details");
            return null;
        }

        File file = new File(fileDetails[3]);
        long dateModifiedSeconds = TimeUnit.MILLISECONDS.toSeconds(file.lastModified());

        ContentValues contentValue = null;
        if (MEDIA_TYPE_IMAGE == type) {
            contentValue = new ContentValues(9);
            contentValue.put(MediaStore.Images.ImageColumns.TITLE, fileDetails[0]);
            contentValue.put(MediaStore.Images.ImageColumns.DISPLAY_NAME, fileDetails[1]);
            contentValue.put(MediaStore.Images.ImageColumns.DATE_TAKEN,
                             Long.valueOf(fileDetails[4]));
            contentValue.put(MediaStore.Images.ImageColumns.MIME_TYPE, fileDetails[2]);
            contentValue.put(MediaStore.Images.ImageColumns.DATE_MODIFIED, dateModifiedSeconds);
            contentValue.put(MediaStore.Images.ImageColumns.DATA, fileDetails[3]);
            contentValue.put(MediaStore.MediaColumns.WIDTH, width);
            contentValue.put(MediaStore.MediaColumns.HEIGHT, height);
        } else if (MEDIA_TYPE_VIDEO == type) {
            contentValue = new ContentValues(9);
            contentValue.put(MediaStore.Video.Media.TITLE, fileDetails[0]);
            contentValue.put(MediaStore.Video.Media.DISPLAY_NAME, fileDetails[1]);
            contentValue.put(MediaStore.Video.Media.DATE_TAKEN, Long.valueOf(fileDetails[4]));
            contentValue.put(MediaStore.MediaColumns.DATE_MODIFIED,
                             Long.valueOf(fileDetails[4]) / 1000);
            contentValue.put(MediaStore.Video.Media.MIME_TYPE, fileDetails[2]);
            contentValue.put(MediaStore.Video.Media.DATA, fileDetails[3]);
            contentValue.put(MediaStore.Video.Media.WIDTH, width);
            contentValue.put(MediaStore.Video.Media.HEIGHT, height);
            contentValue.put(MediaStore.Video.Media.RESOLUTION,
                             Integer.toString(width) + "x" + Integer.toString(height));
        }
        return contentValue;
    }
}
