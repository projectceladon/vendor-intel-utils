/*
 * Copyright (C) 2014 The Android Open Source Project
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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.*;
import android.content.res.Resources;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Point;
import android.net.Uri;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.StatFs;
import android.provider.MediaStore;
import android.text.format.DateUtils;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import javax.microedition.khronos.opengles.GL11;

public class Utils {
    private static final String TAG = "Utils";

    public static final String DCIM =
            Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();
    public static final String DIRECTORY = DCIM + "/MultiCamera";

    public static final int MEDIA_TYPE_IMAGE = 0;
    public static final int MEDIA_TYPE_VIDEO = 1;

    public static final String IMAGE_FILE_NAME_FORMAT = "'IMG'_yyyyMMdd_HHmmss";
    public static final String VIDEO_FILE_NAME_FORMAT = "'VID'_yyyyMMdd_HHmmss";

    /**
     * See android.hardware.Camera.ACTION_NEW_PICTURE.
     */
    public static final String ACTION_NEW_PICTURE = "android.hardware.action.NEW_PICTURE";
    /**
     * See android.hardware.Camera.ACTION_NEW_VIDEO.
     */
    public static final String ACTION_NEW_VIDEO = "android.hardware.action.NEW_VIDEO";

    private static final String VIDEO_BASE_URI = "content://media/external/video/media";

    private static final int MAX_PEEK_BITMAP_PIXELS = 1600000;  // 1.6 * 4 MBs.
    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;
    public static final long LOW_STORAGE_THRESHOLD_BYTES = 50000000;

    private static Uri mCurrentPictureUri, mCurrentVideoUri;

    /**
     * Has to be in sync with the receiving MovieActivity.
     */
    public static final String KEY_TREAT_UP_AS_BACK = "treat-up-as-back";

    private static final int DOWN_SAMPLE_FACTOR = 4;
    private static ContentValues mCurrentPicInfo, mCurrentVideoInfo;

    @SuppressLint("SimpleDateFormat")
    public static File createOutputmediaStorageDir() {
        // To be safe, you should check that the SDCard is mounted
        // using Environment.getExternalStorageState() before doing this.

        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            Log.e(TAG, "getExternalStorageState  failed");
            return null;
        }

        File mediaStorageDir = new File(DIRECTORY);
        // This location works best if you want the created images to be shared
        // between applications and persist after your app has been uninstalled.

        // Create the storage directory if it does not exist
        if (!mediaStorageDir.exists()) {
            if (!mediaStorageDir.mkdirs()) {
                Log.e(TAG, "Failed to create directory for " + DIRECTORY);
                return null;
            }
        }

        return mediaStorageDir;
    }

    public static Uri broadcastNewPicture(Context context, ContentValues values) {
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
        return uri;
    }

    public static Uri broadcastNewVideo(Context context, ContentValues values) {
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
        return uri;
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

    public static ContentValues getContentValues(int type, String[] fileDetails, int width,
                                                 int height, long duration, long size) {
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
            contentValue.put(MediaStore.Images.ImageColumns.SIZE, size);

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
            contentValue.put(MediaStore.Video.Media.DURATION, duration);
            contentValue.put(MediaStore.Video.Media.SIZE, size);
        }
        return contentValue;
    }

    /**
     * Returns the maximum video recording duration (in milliseconds).
     */
    public static int getMaxVideoDuration(Context context) {
        int duration = 0;  // in milliseconds, 0 means unlimited.
        try {
            duration =
                    0;  // context.getResources().getInteger(R.integer.max_video_recording_length);
        } catch (Resources.NotFoundException ex) {
        }
        return duration;
    }

    public static String millisecondToTimeString(long milliSeconds, boolean displayCentiSeconds) {
        long seconds = milliSeconds / 1000;  // round down to compute seconds
        long minutes = seconds / 60;
        long hours = minutes / 60;
        long remainderMinutes = minutes - (hours * 60);
        long remainderSeconds = seconds - (minutes * 60);

        StringBuilder timeStringBuilder = new StringBuilder();

        // Hours
        if (hours > 0) {
            if (hours < 10) {
                timeStringBuilder.append('0');
            }
            timeStringBuilder.append(hours);

            timeStringBuilder.append(':');
        }

        // Minutes
        if (remainderMinutes < 10) {
            timeStringBuilder.append('0');
        }
        timeStringBuilder.append(remainderMinutes);
        timeStringBuilder.append(':');

        // Seconds
        if (remainderSeconds < 10) {
            timeStringBuilder.append('0');
        }
        timeStringBuilder.append(remainderSeconds);

        // Centi seconds
        if (displayCentiSeconds) {
            timeStringBuilder.append('.');
            long remainderCentiSeconds = (milliSeconds - seconds * 1000) / 10;
            if (remainderCentiSeconds < 10) {
                timeStringBuilder.append('0');
            }
            timeStringBuilder.append(remainderCentiSeconds);
        }

        return timeStringBuilder.toString();
    }

    /**
     * Load the thumbnail of an image from an {@link InputStream}.
     *
     * @param stream        The input stream of the image.
     * @param imageWidth    Image width.
     * @param imageHeight   Image height.
     * @param widthBound    The bound of the width of the decoded image.
     * @param heightBound   The bound of the height of the decoded image.
     * @param orientation   The orientation of the image. The image will be rotated
     *                      clockwise in degrees.
     * @param maximumPixels The bound for the number of pixels of the decoded image.
     * @return {@code null} if the decoding failed.
     */
    public static Bitmap loadImageThumbnailFromStream(InputStream stream, int imageWidth,
                                                      int imageHeight, int widthBound,
                                                      int heightBound, int orientation,
                                                      int maximumPixels) {
        /** 32K buffer. */
        byte[] decodeBuffer = new byte[32 * 1024];

        if (orientation % 180 != 0) {
            int dummy = imageHeight;
            imageHeight = imageWidth;
            imageWidth = dummy;
        }

        // Generate Bitmap of maximum size that fits into widthBound x heightBound.
        // Algorithm: start with full size and step down in powers of 2.
        int targetWidth = imageWidth;
        int targetHeight = imageHeight;
        int sampleSize = 1;
        while (targetHeight > heightBound || targetWidth > widthBound ||
               targetHeight > GL11.GL_MAX_TEXTURE_SIZE || targetWidth > GL11.GL_MAX_TEXTURE_SIZE ||
               targetHeight * targetWidth > maximumPixels) {
            sampleSize <<= 1;
            targetWidth = imageWidth / sampleSize;
            targetHeight = imageWidth / sampleSize;
        }

        // For large (> MAXIMUM_TEXTURE_SIZE) high aspect ratio (panorama)
        // Bitmap requests:
        //   Step 1: ask for double size.
        //   Step 2: scale maximum edge down to MAXIMUM_TEXTURE_SIZE.
        //
        // Here's the step 1: double size.
        if ((heightBound > GL11.GL_MAX_TEXTURE_SIZE || widthBound > GL11.GL_MAX_TEXTURE_SIZE) &&
            targetWidth * targetHeight < maximumPixels / 4 && sampleSize > 1) {
            sampleSize >>= 2;
        }

        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inSampleSize = sampleSize;
        opts.inTempStorage = decodeBuffer;
        Bitmap b = BitmapFactory.decodeStream(stream, null, opts);

        if (b == null) {
            return null;
        }

        // Step 2: scale maximum edge down to maximum texture size.
        // If Bitmap maximum edge > MAXIMUM_TEXTURE_SIZE, which can happen for panoramas,
        // scale to fit in MAXIMUM_TEXTURE_SIZE.
        if (b.getWidth() > GL11.GL_MAX_TEXTURE_SIZE || b.getHeight() > GL11.GL_MAX_TEXTURE_SIZE) {
            int maxEdge = Math.max(b.getWidth(), b.getHeight());
            b = Bitmap.createScaledBitmap(b, b.getWidth() * GL11.GL_MAX_TEXTURE_SIZE / maxEdge,
                                          b.getHeight() * GL11.GL_MAX_TEXTURE_SIZE / maxEdge,
                                          false);
        }

        // Not called often because most modes save image data non-rotated.
        if (orientation != 0 && b != null) {
            Matrix m = new Matrix();
            m.setRotate(orientation);
            b = Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(), m, false);
        }

        return b;
    }

    public static Optional<Bitmap> generateThumbnail(File path, int boundingWidthPx,
                                                     int boundingHeightPx) {
        final Bitmap bitmap;

        /*if (getAttributes().isRendering()) {
            return Storage.getPlaceholderForSession(data.getUri());
        } else {*/

        FileInputStream stream;

        try {
            stream = new FileInputStream(path);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "### File not found ###:" + path.getPath());
            return Optional.empty();
        }
        int width = 1280;
        int height = 720;  //.getDimensions().getHeight();
        int orientation = 0;

        Point dim = resizeToFill(width, height, orientation, boundingWidthPx, boundingHeightPx);

        // If the orientation is not vertical
        if (orientation % 180 != 0) {
            int dummy = dim.x;
            dim.x = dim.y;
            dim.y = dummy;
        }

        bitmap = loadImageThumbnailFromStream(stream, width, height, (int)(dim.x * 0.7f),
                                              (int)(dim.y * 0.7), 0, MAX_PEEK_BITMAP_PIXELS);

        return Optional.ofNullable(bitmap);
        //}
    }

    /**
     * Calculates a new dimension to fill the bound with the original aspect
     * ratio preserved.
     *
     * @param imageWidth    The original width.
     * @param imageHeight   The original height.
     * @param imageRotation The clockwise rotation in degrees of the image which
     *                      the original dimension comes from.
     * @param boundWidth    The width of the bound.
     * @param boundHeight   The height of the bound.
     * @returns The final width/height stored in Point.x/Point.y to fill the
     * bounds and preserve image aspect ratio.
     */
    public static Point resizeToFill(int imageWidth, int imageHeight, int imageRotation,
                                     int boundWidth, int boundHeight) {
        if (imageRotation % 180 != 0) {
            // Swap width and height.
            int savedWidth = imageWidth;
            imageWidth = imageHeight;
            imageHeight = savedWidth;
        }

        Point p = new Point();
        p.x = boundWidth;
        p.y = boundHeight;

        // In some cases like automated testing, image height/width may not be
        // loaded, to avoid divide by zero fall back to provided bounds.
        if (imageWidth != 0 && imageHeight != 0) {
            if (imageWidth * boundHeight > boundWidth * imageHeight) {
                p.y = imageHeight * p.x / imageWidth;
            } else {
                p.x = imageWidth * p.y / imageHeight;
            }
        } else {
            Log.w(TAG, "zero width/height, falling back to bounds (w|h|bw|bh):" + imageWidth + "|" +
                               imageHeight + "|" + boundWidth + "|" + boundHeight);
        }

        return p;
    }

    /**
     * Rotates and/or mirrors the bitmap. If a new bitmap is created, the
     * original bitmap is recycled.
     */
    public static Bitmap rotateAndMirror(Bitmap b, int degrees, boolean mirror) {
        if ((degrees != 0 || mirror) && b != null) {
            Matrix m = new Matrix();
            // Mirror first.
            // horizontal flip + rotation = -rotation + horizontal flip
            if (mirror) {
                m.postScale(-1, 1);
                degrees = (degrees + 360) % 360;
                if (degrees == 0 || degrees == 180) {
                    m.postTranslate(b.getWidth(), 0);
                } else if (degrees == 90 || degrees == 270) {
                    m.postTranslate(b.getHeight(), 0);
                } else {
                    throw new IllegalArgumentException("Invalid degrees=" + degrees);
                }
            }
            if (degrees != 0) {
                // clockwise
                m.postRotate(degrees, (float)b.getWidth() / 2, (float)b.getHeight() / 2);
            }

            try {
                Bitmap b2 = Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(), m, true);
                if (b != b2) {
                    b.recycle();
                    b = b2;
                }
            } catch (OutOfMemoryError ex) {
                // We have no memory to rotate. Return the original bitmap.
            }
        }
        return b;
    }

    public static Optional<Bitmap> getVideoThumbnail(ContentResolver mContentResolver, Uri uri) {
        Bitmap bitmap = null;
        ParcelFileDescriptor mVideoFileDescriptor;

        try {
            mVideoFileDescriptor = mContentResolver.openFileDescriptor(uri, "r");
            bitmap = Thumbnail.createVideoThumbnailBitmap(mVideoFileDescriptor.getFileDescriptor(),
                                                          720);
        } catch (java.io.FileNotFoundException ex) {
            // invalid uri
            Log.e(TAG, ex.toString());
        }

        if (bitmap != null) {
            // MetadataRetriever already rotates the thumbnail. We should rotate
            // it to match the UI orientation (and mirror if it is front-facing camera).
            bitmap = rotateAndMirror(bitmap, 0, false);
        }
        return Optional.ofNullable(bitmap);
    }

    public static Intent getVideoPlayerIntent(Uri uri) {
        return new Intent(Intent.ACTION_VIEW).setDataAndType(uri, "video/*");
    }

    public static void playVideo(Activity activity, Uri uri, String title) {
        try {
            Intent intent = getVideoPlayerIntent(uri)
                                    .putExtra(Intent.EXTRA_TITLE, title)
                                    .putExtra(KEY_TREAT_UP_AS_BACK, true);
            activity.startActivity(intent);

        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "cant play video");
        }
    }

    public static String getRealPathFromURI(Context context, Uri contentUri) {
        Cursor cursor = null;
        int column_index;
        try {
            String[] proj = {MediaStore.Images.Media.DATA, MediaStore.Video.Media.DATA};
            cursor = context.getContentResolver().query(contentUri, proj, null, null, null);

            if (getMimeTypeFromURI(context, contentUri).compareTo("video/mp4") == 0) {
                column_index = cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA);
                cursor.moveToFirst();

            } else {
                column_index = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                cursor.moveToFirst();
            }

            return cursor.getString(column_index);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    public static String getMimeTypeFromURI(Context context, Uri uri) {
        ContentResolver cR = context.getContentResolver();
        String type = cR.getType(uri);
        return type;
    }

    public static Optional<MediaDetails> getMediaDetails(Context mContext, ContentValues info) {
        MediaDetails mediaDetails = new MediaDetails();
        final DateFormat mDateFormatter = DateFormat.getDateTimeInstance();

        if (info.get(MediaStore.Video.Media.MIME_TYPE).equals("video/mp4") == true) {
            File file = new File(info.getAsString(MediaStore.Video.Media.DATA));

            mediaDetails.addDetail(MediaDetails.INDEX_TITLE,
                                   info.get(MediaStore.Video.Media.TITLE));
            mediaDetails.addDetail(MediaDetails.INDEX_PATH, info.get(MediaStore.Video.Media.DATA));
            long mSizeInBytes = info.getAsLong(MediaStore.Video.Media.SIZE);
            if (mSizeInBytes > 0) {
                mediaDetails.addDetail(MediaDetails.INDEX_SIZE, mSizeInBytes);
            }

            String Dimensions = MediaDetails.getDimentions(
                    mContext, info.getAsInteger(MediaStore.Video.Media.WIDTH),
                    info.getAsInteger(MediaStore.Video.Media.HEIGHT));

            mediaDetails.addDetail(MediaDetails.INDEX_DIMENSIONS, Dimensions);

            mediaDetails.addDetail(MediaDetails.INDEX_TYPE,
                                   info.get(MediaStore.Video.Media.MIME_TYPE));

            String dateModified = DateUtils.formatDateTime(
                    mContext, info.getAsLong(MediaStore.Video.Media.DATE_TAKEN),
                    DateUtils.FORMAT_SHOW_TIME | DateUtils.FORMAT_SHOW_DATE |
                            DateUtils.FORMAT_SHOW_YEAR | DateUtils.FORMAT_ABBREV_ALL);

            mediaDetails.addDetail(MediaDetails.INDEX_DATETIME, dateModified);

            String duration = MediaDetails.formatDuration(
                    mContext, TimeUnit.MILLISECONDS.toSeconds(
                                      (Long)info.get(MediaStore.Video.Media.DURATION)));
            mediaDetails.addDetail(MediaDetails.INDEX_DURATION, duration);

        } else if (info.get(MediaStore.Video.Media.MIME_TYPE).equals("image/jpeg") == true) {
            File file = new File(info.getAsString(MediaStore.Images.ImageColumns.DATA));
            mediaDetails.addDetail(MediaDetails.INDEX_TITLE,
                                   info.get(MediaStore.Images.ImageColumns.TITLE));
            mediaDetails.addDetail(MediaDetails.INDEX_PATH,
                                   info.get(MediaStore.Images.ImageColumns.DATA));
            String Dimensions = MediaDetails.getDimentions(
                    mContext, info.getAsInteger(MediaStore.MediaColumns.WIDTH),
                    info.getAsInteger(MediaStore.MediaColumns.HEIGHT));

            mediaDetails.addDetail(MediaDetails.INDEX_DIMENSIONS, Dimensions);

            mediaDetails.addDetail(MediaDetails.INDEX_TYPE,
                                   info.get(MediaStore.Images.ImageColumns.MIME_TYPE));

            String dateModified = DateUtils.formatDateTime(
                    mContext, info.getAsLong(MediaStore.Images.ImageColumns.DATE_TAKEN),
                    DateUtils.FORMAT_SHOW_TIME | DateUtils.FORMAT_SHOW_DATE |
                            DateUtils.FORMAT_SHOW_YEAR | DateUtils.FORMAT_ABBREV_ALL);
            mediaDetails.addDetail(MediaDetails.INDEX_DATETIME, dateModified);

            long mSizeInBytes = info.getAsLong(MediaStore.Images.ImageColumns.SIZE);
            if (mSizeInBytes > 0) {
                mediaDetails.addDetail(MediaDetails.INDEX_SIZE, mSizeInBytes);
            }
        }

        return Optional.of(mediaDetails);
    }

    public static long getAvailableSpace() {
        String state = Environment.getExternalStorageState();
        Log.d(TAG, "External storage state=" + state);
        if (Environment.MEDIA_CHECKING.equals(state)) {
            return PREPARING;
        }
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return UNAVAILABLE;
        }

        File dir = createOutputmediaStorageDir();
        if (!dir.isDirectory() || !dir.canWrite()) {
            return UNAVAILABLE;
        }

        try {
            StatFs stat = new StatFs(DIRECTORY);
            return stat.getAvailableBlocksLong() * stat.getBlockSizeLong();
        } catch (Exception e) {
            Log.i(TAG, "Fail to access external storage", e);
        }
        return UNKNOWN_SIZE;
    }
}
