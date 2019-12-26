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

import android.app.Activity;
import android.app.Dialog;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.CamcorderProfile;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.util.Log;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceManager;
import java.io.*;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;

public class CameraBase implements MediaRecorder.OnErrorListener, MediaRecorder.OnInfoListener {
    private Activity mActivity;
    private String TAG;
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView textureView;
    private ImageButton FullScrn, SettingsView, takePictureButton, TakeVideoButton;

    private MediaRecorder mMediaRecorder;
    private String cameraId;
    private CameraDevice cameraDevice;
    private CameraCaptureSession cameraCaptureSessions;
    private CaptureRequest.Builder captureRequestBuilder;
    private Size previewSize;
    private ImageReader imageReader;
    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;
    private static final int SENSOR_ORIENTATION_DEFAULT_DEGREES = 90;
    private static final int SENSOR_ORIENTATION_INVERSE_DEGREES = 270;
    private static final SparseIntArray DEFAULT_ORIENTATIONS = new SparseIntArray();
    private static final SparseIntArray INVERSE_ORIENTATIONS = new SparseIntArray();
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    private SharedPreferences settings;
    private SurfaceTexture mSurfaceTexture;
    private String Capture_Key, Video_key, SettingsKey;
    private long mRecordingStartTime;
    private boolean mRecordingTimeCountsDown = false;
    private static final int MSG_UPDATE_RECORD_TIME = 5;
    private static final String SIZE_HD = "HD 720p";

    /**
     * Whether the app is recording video now
     */
    private boolean mIsRecordingVideo;

    // The video file that the hardware camera is about to record into
    // (or is recording into.
    private String mVideoFilename;
    private ContentValues mCurrentFileInfo, mCurrentVideoValues;
    private final Handler mHandler;
    private TextView mRecordingTimeView;

    /**
     * Orientation of the camera sensor
     */
    private int mSensorOrientation;

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    static final Size SIZE_720P = new Size(1280, 720);
    static final Size SIZE_480P = new Size(640, 480);

    private RoundedThumbnailView mRoundedThumbnailView;
    FrameLayout roundedThumbnailViewControlLayout;

    private Uri mCurrentUri;
    private String[] ImageFileDetails;

    public CameraBase(Activity activity, AutoFitTextureView mtextureView, ImageButton[] Button,
                      TextView RecordingTimeView, String[] data,
                      RoundedThumbnailView roundedThumbnailView) {
        this.mActivity = activity;
        this.textureView = mtextureView;
        this.ClickListeners(Button[0], Button[1]);
        SettingsView = Button[2];
        FullScrn = Button[3];
        this.settings = PreferenceManager.getDefaultSharedPreferences(activity);
        cameraId = data[1];
        TAG = data[0];
        Capture_Key = data[2];
        Video_key = data[3];
        SettingsKey = data[4];
        mHandler = new MainHandler();

        mRecordingTimeView = RecordingTimeView;

        mRoundedThumbnailView = roundedThumbnailView;

        RoundedThumbnail_setOnClickListners();

        roundedThumbnailViewControlLayout = mActivity.findViewById(R.id.control1);

        Log.v(TAG, "constructor called");
    }

    private void RoundedThumbnail_setOnClickListners() {
        mRoundedThumbnailView.setCallback(new RoundedThumbnailView.Callback() {
            @Override
            public void onHitStateFinished() {
                ImageView preView;
                final ImageButton btnDelete, playButton, btnBack, details;
                FrameLayout previewLayout;

                String mimeType = Utils.getMimeTypeFromURI(mActivity, mCurrentUri);

                previewLayout = mActivity.findViewById(R.id.previewLayout);
                previewLayout.setVisibility(View.VISIBLE);

                btnDelete = mActivity.findViewById(R.id.control_delete);
                playButton = mActivity.findViewById(R.id.play_button);
                preView = mActivity.findViewById(R.id.preview);
                btnBack = mActivity.findViewById(R.id.control_back);
                details = mActivity.findViewById(R.id.control_info);
                details.setVisibility(View.VISIBLE);

                btnBack.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        FrameLayout previewLayout;
                        previewLayout = mActivity.findViewById(R.id.previewLayout);
                        previewLayout.setVisibility(View.GONE);

                        mRoundedThumbnailView.hideThumbnail();
                    }
                });

                btnDelete.setOnClickListener(new View.OnClickListener() {
                    ImageView preView;

                    @Override
                    public void onClick(View v) {
                        preView = mActivity.findViewById(R.id.preview);

                        Uri uri = mCurrentUri;
                        File file = new File(Utils.getRealPathFromURI(mActivity, uri));
                        if (file.exists()) {
                            Log.v(TAG, " Thumbnail Preview File Deleted ");
                            file.delete();
                            // request scan
                            Intent scanIntent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                            scanIntent.setData(Uri.fromFile(file));
                            mActivity.sendBroadcast(scanIntent);
                            preView.setImageResource(android.R.color.background_dark);
                            details.setVisibility(View.INVISIBLE);
                        }
                    }
                });

                details.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        showDetailsDialog(mCurrentFileInfo);
                    }
                });

                if (mimeType.compareTo("video/mp4") == 0) {
                    VideoPreview(playButton, preView);

                } else {
                    photoPreview(playButton, preView);
                }
            }
        });
    }

    private void VideoPreview(ImageButton playButton, ImageView preView) {
        final Optional<Bitmap> bitmap =
                Utils.getVideoThumbnail(mActivity.getContentResolver(), mCurrentUri);
        if (bitmap.isPresent()) {
            playButton.setVisibility(View.VISIBLE);

            playButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Utils.playVideo(mActivity, mCurrentUri, TAG);
                }
            });

            preView.setVisibility(View.VISIBLE);
            preView.setImageBitmap(bitmap.get());

        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }

    private void photoPreview(ImageButton playButton, ImageView preView) {
        Uri PhotoUri = mCurrentUri;

        preView.setVisibility(View.VISIBLE);
        playButton.setVisibility(View.GONE);
        preView.setImageURI(PhotoUri);
    }

    /**
     * This Handler is used to post message back onto the main thread of the
     * application.
     */
    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_UPDATE_RECORD_TIME: {
                    updateRecordingTime();
                    break;
                }

                default:
                    Log.v(TAG, "Unhandled message: " + msg.what);
                    break;
            }
        }
    }

    private void updateRecordingTime() {
        if (!mIsRecordingVideo) {
            return;
        }
        long now = SystemClock.uptimeMillis();
        long delta = now - mRecordingStartTime;
        long mMaxVideoDurationInMs;
        mMaxVideoDurationInMs = Utils.getMaxVideoDuration(mActivity);

        // Starting a minute before reaching the max duration
        // limit, we'll countdown the remaining time instead.
        boolean countdownRemainingTime =
                (mMaxVideoDurationInMs != 0 && delta >= mMaxVideoDurationInMs - 60000);

        long deltaAdjusted = delta;
        if (countdownRemainingTime) {
            deltaAdjusted = Math.max(0, mMaxVideoDurationInMs - deltaAdjusted) + 999;
        }
        String text;

        long targetNextUpdateDelay;

        text = Utils.millisecondToTimeString(deltaAdjusted, false);
        targetNextUpdateDelay = 1000;

        setRecordingTime(text);

        if (mRecordingTimeCountsDown != countdownRemainingTime) {
            // Avoid setting the color on every update, do it only
            // when it needs changing.
            mRecordingTimeCountsDown = countdownRemainingTime;

            int color = mActivity.getResources().getColor(R.color.recording_time_remaining_text);

            setRecordingTimeTextColor(color);
        }

        long actualNextUpdateDelay = targetNextUpdateDelay - (delta % targetNextUpdateDelay);
        mHandler.sendEmptyMessageDelayed(MSG_UPDATE_RECORD_TIME, actualNextUpdateDelay);
    }

    private void setRecordingTime(String text) {
        mRecordingTimeView.setText(text);
    }

    private void setRecordingTimeTextColor(int color) {
        mRecordingTimeView.setTextColor(color);
    }

    private void showRecordingUI(boolean recording) {
        if (recording) {
            mRecordingTimeView.setText("");
            mRecordingTimeView.setVisibility(View.VISIBLE);
            mRecordingTimeView.announceForAccessibility(
                    mActivity.getResources().getString(R.string.video_recording_started));

        } else {
            mRecordingTimeView.announceForAccessibility(
                    mActivity.getResources().getString(R.string.video_recording_stopped));
            mRecordingTimeView.setVisibility(View.GONE);
        }
    }

    private void ClickListeners(ImageButton PictureButton, ImageButton RecordButton) {
        TakePicureOnClicked(PictureButton);

        StartVideoRecording(RecordButton);
    }

    private void TakePicureOnClicked(ImageButton PictureButton) {
        takePictureButton = PictureButton;
        if (takePictureButton == null) return;

        takePictureButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                takePicture();
            }
        });
    }

    private void StartVideoRecording(ImageButton RecordButton) {
        TakeVideoButton = RecordButton;

        TakeVideoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mIsRecordingVideo) {
                    stopRecordingVideo();

                } else {
                    startRecordingVideo();
                }
            }
        });
    }

    TextureView.SurfaceTextureListener textureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            // open your camera here
            mSurfaceTexture = surface;
            openCamera(width, height);
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            // Transform you image captured size according to the surface width and height

            adjustAspectRatio(previewSize.getWidth(), previewSize.getHeight());
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            return false;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        }
    };

    private void openCamera(int width, int height) {
        CameraManager manager = (CameraManager)mActivity.getSystemService(Context.CAMERA_SERVICE);
        try {
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) return;

            String Key = GetChnagedPrefKey();

            previewSize = getSelectedDimension(Key);

            configureTransform(width, height);

            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);

            startBackgroundThread();

            manager.openCamera(cameraId, stateCallback, null);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private final CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            // This is called when the camera is open
            Log.i(TAG, "Camera opened successfully with id " + camera.getId());
            cameraDevice = camera;
            UI_Enable(true);
            createCameraPreview();
            MultiViewActivity.updateStorageSpace(null);
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            Log.v(TAG, "onDisconnected");
            closeCamera();
        }

        @Override
        public void onError(CameraDevice camera, int error) {
            Log.v(TAG, "onError");
            closeCamera();
        }

        @Override
        public void onClosed(@NonNull CameraDevice camera) {
            Log.v(TAG, "onClose");
            super.onClosed(camera);
            previewSize = SIZE_720P;
            configureTransform(textureView.getWidth(), textureView.getHeight());
            SurfaceUtil.clear(mSurfaceTexture);
            UI_Enable(false);
            ResetResolutionSettings();
        }
    };

    private void ResetResolutionSettings() {
        SharedPreferences.Editor edit = settings.edit();
        edit.remove(Capture_Key);
        edit.remove(Video_key);
        edit.apply();
    }

    private void UI_Enable(boolean flag) {
        TakeVideoButton.setEnabled(flag);

        takePictureButton.setEnabled(flag);

        SettingsView.setEnabled(flag);

        FullScrn.setEnabled(flag);

        if (flag) {
            TakeVideoButton.setImageResource(R.drawable.ic_capture_video);
            takePictureButton.setImageResource(R.drawable.ic_capture_camera_normal);
            SettingsView.setImageAlpha(200);
            FullScrn.setImageAlpha(200);
        } else {
            TakeVideoButton.setImageResource(R.drawable.ic_capture_video_disabled);
            takePictureButton.setImageResource(R.drawable.ic_capture_camera_disabled);
            SettingsView.setImageAlpha(95);
            FullScrn.setImageAlpha(95);
        }
    }

    private void configureTransform(int viewWidth, int viewHeight) {
        if (null == textureView || null == previewSize) {
            return;
        }
        int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        Matrix matrix = new Matrix();
        RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
        Log.v(TAG, "configureTransform() viewWidth: " + viewWidth + " viewHeight: " + viewHeight +
                           "previewWidth: " + previewSize.getWidth() +
                           "previewHeight:" + previewSize.getHeight());

        RectF bufferRect = new RectF(0, 0, previewSize.getHeight(), previewSize.getWidth());
        float centerX = viewRect.centerX();
        float centerY = viewRect.centerY();
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
            float scale = Math.max((float)viewHeight / previewSize.getHeight(),
                                   (float)viewWidth / previewSize.getWidth());
            matrix.postScale(scale, scale, centerX, centerY);
            matrix.postRotate(90 * (rotation - 2), centerX, centerY);
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180, centerX, centerY);
        }
        textureView.setTransform(matrix);
    }

    /**
     * Retrieve a setting's value as a String, manually specifiying
     * a default value.
     */
    private String getString(String key, String defaultValue) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(mActivity);
        try {
            return preferences.getString(key, defaultValue);
        } catch (ClassCastException e) {
            Log.w(TAG, "existing preference with invalid type,removing and returning default", e);
            preferences.edit().remove(key).apply();
            return defaultValue;
        }
    }

    private String GetChnagedPrefKey() {
        String Key = null;

        switch (SettingsKey) {
            case "pref_resolution":
                Key = getString(SettingsKey, "capture_list");
                break;
            case "pref_resolution_1":
                Key = getString(SettingsKey, "capture_list_1");
                break;
            case "pref_resolution_2":
                Key = getString(SettingsKey, "capture_list_2");
                break;
            case "pref_resolution_3":
                Key = getString(SettingsKey, "capture_list_3");
                break;
            default:
                break;
        }

        return Key;
    }

    private Size getSelectedDimension(String Key) {
        settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

        Size mDimensions = SIZE_720P;

        if (Key.compareTo(Video_key) == 0) {
            CamcorderProfile mProfile;

            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

            String videoQuality = settings.getString(Video_key, SIZE_HD);

            int quality = SettingsPrefUtil.getFromSetting(videoQuality);

            mProfile = CamcorderProfile.get(0, quality);

            mDimensions = new Size(mProfile.videoFrameWidth, mProfile.videoFrameHeight);

        } else {
            mDimensions = SettingsPrefUtil.sizeFromSettingString(
                    settings.getString(Capture_Key, "1280x720"));
        }

        return mDimensions;
    }

    public void createCameraPreview() {
        try {
            closePreviewSession();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;

            Surface surface = new Surface(texture);

            String Key = GetChnagedPrefKey();

            previewSize = getSelectedDimension(Key);

            if (previewSize.getWidth() == 640 || previewSize.getWidth() == 320) {
                previewSize = SIZE_480P;

            } else if (previewSize.getWidth() == 1280 || previewSize.getWidth() == 1920) {
                previewSize = SIZE_720P;
            }

            Log.i(TAG, "Previewing with " + Key + " " + previewSize.getWidth() + " x " +
                               previewSize.getHeight());

            texture.setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());

            adjustAspectRatio(previewSize.getWidth(), previewSize.getHeight());

            captureRequestBuilder =
                    cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            captureRequestBuilder.addTarget(surface);

            cameraDevice.createCaptureSession(
                    Arrays.asList(surface), new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(CameraCaptureSession cameraCaptureSession) {
                            // The camera is already closed
                            if (null == cameraDevice) {
                                return;
                            }
                            // When the session is ready, we start displaying the preview.
                            cameraCaptureSessions = cameraCaptureSession;
                            updatePreview();
                        }

                        @Override
                        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
                            closeCamera();
                            Toast.makeText(mActivity, " Preview Configuration change",
                                           Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void closeCamera() {
        if (mIsRecordingVideo) {
            stopRecordingVideo();
        }

        closePreviewSession();
        if (null != cameraDevice) {
            cameraDevice.close();
            cameraDevice = null;
        }
        if (null != imageReader) {
            imageReader.close();
            imageReader = null;
        }
        releaseMedia();
        stopBackgroundThread();
    }

    /**
     * Starts a background thread and its {@link Handler}.
     */
    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread(TAG);
        mBackgroundThread.start();
        mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
    }

    /**
     * Stops the background thread and its {@link Handler}.
     */
    private void stopBackgroundThread() {
        if (mBackgroundThread != null) {
            mBackgroundThread.quitSafely();
            try {
                mBackgroundThread.join();
                mBackgroundThread = null;
                mBackgroundHandler = null;
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Sets the TextureView transform to preserve the aspect ratio of the video.
     */
    private void adjustAspectRatio(int videoWidth, int videoHeight) {
        float megaPixels = previewSize.getWidth() * previewSize.getHeight() / 1000000f;
        int numerator = SettingsPrefUtil.aspectRatioNumerator(previewSize);
        int denominator = SettingsPrefUtil.aspectRatioDenominator(previewSize);
        String AspectRatio =
                mActivity.getString(R.string.metadata_dimensions_format, previewSize.getWidth(),
                                    previewSize.getHeight(), megaPixels, numerator, denominator);
        Log.i(TAG, "AspectRatio is " + AspectRatio);

        if (videoWidth >= 1280 || videoHeight >= 720) {
            configureTransform(textureView.getWidth(), textureView.getHeight());
            return;
        }

        int viewWidth = textureView.getWidth();
        int viewHeight = textureView.getHeight();
        double aspectRatio = (double)videoHeight / videoWidth;

        int newWidth, newHeight;
        if (viewHeight > (int)(viewWidth * aspectRatio)) {
            // limited by narrow width; restrict height
            newWidth = viewWidth;
            newHeight = (int)(viewWidth * aspectRatio);
        } else {
            // limited by short height; restrict width
            newWidth = (int)(viewHeight / aspectRatio);
            newHeight = viewHeight;
        }
        int xoff = (viewWidth - newWidth) / 2;
        int yoff = (viewHeight - newHeight) / 2;
        Log.v(TAG, "video=" + videoWidth + "x" + videoHeight + " view=" + viewWidth + "x" +
                           viewHeight + " newView=" + newWidth + "x" + newHeight + " off=" + xoff +
                           "," + yoff);

        Matrix txform = new Matrix();
        textureView.getTransform(txform);
        txform.setScale((float)newWidth / viewWidth, (float)newHeight / viewHeight);
        // txform.postRotate(10);          // just for fun
        txform.postTranslate(xoff, yoff);
        textureView.setTransform(txform);
    }

    private void updatePreview() {
        if (null == cameraDevice) {
            Log.e(TAG, "updatePreview error");
        }
        captureRequestBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
        HandlerThread thread = new HandlerThread("Camera Preview");
        thread.start();
        Handler handler = new Handler(thread.getLooper());
        try {
            cameraCaptureSessions.setRepeatingRequest(captureRequestBuilder.build(), null, handler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /**
     * Retrieves the JPEG orientation from the specified screen rotation.
     *
     * @param rotation The screen rotation.
     * @return The JPEG orientation (one of 0, 90, 270, and 360)
     */
    private int getOrientation(int rotation) {
        // Sensor orientation is 90 for most devices, or 270 for some devices (eg. Nexus 5X)
        // We have to take that into account and rotate JPEG properly.
        // For devices with orientation of 90, we simply return our mapping from ORIENTATIONS.
        // For devices with orientation of 270, we need to rotate the JPEG 180 degrees.
        return (ORIENTATIONS.get(rotation) + mSensorOrientation + 270) % 360;
    }

    private void takePicture() {
        if (null == cameraDevice) {
            Log.e(TAG, "cameraDevice is null");
            return;
        }

        try {
            final Size imageDimension = getSelectedDimension(Capture_Key);

            Log.i(TAG, "Still Capture imageDimension " + imageDimension.getWidth() + " x " +
                               imageDimension.getHeight());

            ImageReader reader = ImageReader.newInstance(
                    imageDimension.getWidth(), imageDimension.getHeight(), ImageFormat.JPEG, 1);
            List<Surface> outputSurfaces = new ArrayList<>(2);

            outputSurfaces.add(reader.getSurface());

            captureRequestBuilder =
                    cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureRequestBuilder.addTarget(reader.getSurface());
            captureRequestBuilder.set(CaptureRequest.CONTROL_MODE,
                                      CameraMetadata.CONTROL_MODE_AUTO);
            // Orientation
            int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();

            captureRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, getOrientation(rotation));

            ImageFileDetails = Utils.generateFileDetails(Utils.MEDIA_TYPE_IMAGE);
            if (ImageFileDetails == null || ImageFileDetails.length < 5) {
                Log.e(TAG, "takePicture Invalid file details");
                return;
            }
            String mPictureFilename = ImageFileDetails[3];

            final File ImageFile = new File(mPictureFilename);

            ImageReader.OnImageAvailableListener readerListener =
                    new ImageReader.OnImageAvailableListener() {
                        @Override
                        public void onImageAvailable(ImageReader reader) {
                            Image image = null;
                            try {
                                image = reader.acquireLatestImage();
                                ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                                byte[] bytes = new byte[buffer.capacity()];
                                buffer.get(bytes);

                                save(bytes);
                            } catch (FileNotFoundException e) {
                                e.printStackTrace();
                            } catch (IOException e) {
                                e.printStackTrace();
                            } finally {
                                if (image != null) {
                                    image.close();
                                }
                            }
                        }

                        private void save(byte[] bytes) throws IOException {
                            OutputStream output = null;
                            try {
                                output = new FileOutputStream(ImageFile);
                                output.write(bytes);
                            } finally {
                                if (null != output) {
                                    output.close();
                                }
                            }
                        }
                    };
            reader.setOnImageAvailableListener(readerListener, mBackgroundHandler);
            final CameraCaptureSession.CaptureCallback captureListener =
                    new CameraCaptureSession.CaptureCallback() {
                        @Override
                        public void onCaptureCompleted(CameraCaptureSession session,
                                                       CaptureRequest request,
                                                       TotalCaptureResult result) {
                            super.onCaptureCompleted(session, request, result);

                            saveImage(imageDimension, ImageFile);

                            showImageThumbnail(ImageFile);

                            createCameraPreview();
                        }
                    };
            cameraDevice.createCaptureSession(
                    outputSurfaces, new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(CameraCaptureSession session) {
                            try {
                                session.capture(captureRequestBuilder.build(), captureListener,
                                                mBackgroundHandler);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed(CameraCaptureSession session) {
                        }
                    }, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void saveImage(Size imageDimension, File ImageFile) {
        ContentValues mCurrentPictureValues;
        Uri uri;

        mCurrentPictureValues = Utils.getContentValues(
                Utils.MEDIA_TYPE_IMAGE, ImageFileDetails, imageDimension.getWidth(),
                imageDimension.getHeight(), 0, ImageFile.length());

        uri = Utils.broadcastNewPicture(mActivity.getApplicationContext(), mCurrentPictureValues);

        mCurrentUri = uri;

        mCurrentFileInfo = mCurrentPictureValues;
        Log.i(TAG, "Image saved @ " + ImageFile.getAbsolutePath());
    }

    private void showImageThumbnail(File ImageFile) {
        final int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        final Optional<Bitmap> bitmap =
                Utils.generateThumbnail(ImageFile, roundedThumbnailViewControlLayout.getWidth(),
                                        roundedThumbnailViewControlLayout.getMeasuredHeight());

        if (bitmap.isPresent()) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mRoundedThumbnailView.startRevealThumbnailAnimation("photo taken");
                    mRoundedThumbnailView.setThumbnail(bitmap.get(), getOrientation(rotation));
                }
            });

        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }

    /* Recording Start*/
    private void startRecordingVideo() {
        if (null == cameraDevice || !textureView.isAvailable()) {
            return;
        }
        MultiViewActivity.updateStorageSpace(new MultiViewActivity.OnStorageUpdateDoneListener() {
            @Override
            public void onStorageUpdateDone(long bytes) throws IOException, CameraAccessException {
                if (bytes <= Utils.LOW_STORAGE_THRESHOLD_BYTES) {
                    Log.w(TAG, "Storage issue, ignore the start request");
                    Toast.makeText(mActivity, "LOW_STORAGE", Toast.LENGTH_SHORT).show();
                } else {
                    CamcorderProfile mProfile;
                    mProfile = getSelectedCamorderProfile();
                    setUpMediaRecorder(mProfile);
                    startRecorder(mProfile);
                }
            }
        });
    }

    private CamcorderProfile getSelectedCamorderProfile() {
        CamcorderProfile mProfile;

        settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
        String videoQuality = settings.getString(Video_key, SIZE_HD);

        int quality = SettingsPrefUtil.getFromSetting(videoQuality);

        Log.i(TAG, "Selected video quality " + videoQuality);

        mProfile = CamcorderProfile.get(0, quality);

        return mProfile;
    }

    private void startRecorder(CamcorderProfile mProfile)
            throws IOException, CameraAccessException {
        closePreviewSession();

        SurfaceTexture texture = textureView.getSurfaceTexture();
        if (texture == null) return;

        if (mProfile.videoFrameWidth == 1280 || mProfile.videoFrameHeight == 1920) {
            previewSize = SIZE_720P;

        } else if (mProfile.videoFrameWidth == 640 || mProfile.videoFrameHeight == 320) {
            previewSize = SIZE_480P;
        }

        Log.i(TAG,
              "Video previewSize  " + previewSize.getWidth() + " x " + previewSize.getHeight());

        texture.setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());

        adjustAspectRatio(previewSize.getWidth(), previewSize.getHeight());

        captureRequestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
        List<Surface> surfaces = new ArrayList<>();

        // Set up Surface for the camera preview
        Surface previewSurface = new Surface(texture);
        surfaces.add(previewSurface);
        captureRequestBuilder.addTarget(previewSurface);

        // Set up Surface for the MediaRecorder
        Surface recorderSurface = mMediaRecorder.getSurface();
        surfaces.add(recorderSurface);
        captureRequestBuilder.addTarget(recorderSurface);

        // Start a capture session
        // Once the session starts, we can update the UI and start recording
        cameraDevice.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
            @Override
            public void onConfigured(@NonNull CameraCaptureSession camCaptureSession) {
                cameraCaptureSessions = camCaptureSession;
                updatePreview();
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        // UI
                        mIsRecordingVideo = true;

                        // Start recording

                        mMediaRecorder.start();
                        Log.i(TAG, "MediaRecorder recording.... ");

                        mRecordingStartTime = SystemClock.uptimeMillis();

                        VideoUI(mIsRecordingVideo);

                        updateRecordingTime();
                    }
                });
            }

            @Override
            public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                if (null != mActivity) {
                    Log.e(TAG, "Recording Failed");
                    Toast.makeText(mActivity, "Recording Failed", Toast.LENGTH_SHORT).show();
                }

                releaseMedia();
            }
        }, mBackgroundHandler);
    }

    private void setUpMediaRecorder(CamcorderProfile mProfile) throws IOException {
        if (null == mActivity) {
            return;
        }
        String[] VideofileDetails;

        mMediaRecorder = new MediaRecorder();
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);

        {
            Context mContext = mActivity.getApplicationContext();
            AudioManager mAudioManager =
                    (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
            AudioDeviceInfo[] deviceList =
                    mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
            for (AudioDeviceInfo audioDeviceInfo : deviceList) {
                if (audioDeviceInfo.getType() == AudioDeviceInfo.TYPE_USB_DEVICE) {
                    mMediaRecorder.setPreferredDevice(audioDeviceInfo);
                    break;
                }
            }
        }

        VideofileDetails = Utils.generateFileDetails(Utils.MEDIA_TYPE_VIDEO);
        if (VideofileDetails == null || VideofileDetails.length < 5) {
            Log.e(TAG, "setUpMediaRecorder Invalid file details");
            return;
        }

        mCurrentVideoValues =
                Utils.getContentValues(Utils.MEDIA_TYPE_VIDEO, VideofileDetails,
                                       mProfile.videoFrameWidth, mProfile.videoFrameHeight, 0, 0);

        mVideoFilename = VideofileDetails[3];

        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        /**
         * set output file in media recorder
         */
        mMediaRecorder.setOutputFile(mVideoFilename);

        mMediaRecorder.setVideoEncodingBitRate(10000000);
        mMediaRecorder.setVideoFrameRate(mProfile.videoFrameRate);

        Log.i(TAG, "MediaRecorder VideoSize  " + mProfile.videoFrameWidth + " x " +
                           mProfile.videoFrameHeight);

        mMediaRecorder.setVideoSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);

        int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();

        switch (mSensorOrientation) {
            case SENSOR_ORIENTATION_DEFAULT_DEGREES:
                mMediaRecorder.setOrientationHint(DEFAULT_ORIENTATIONS.get(rotation));
                break;
            case SENSOR_ORIENTATION_INVERSE_DEGREES:
                mMediaRecorder.setOrientationHint(INVERSE_ORIENTATIONS.get(rotation));
                break;
        }

        // Set maximum file size.
        long maxFileSize =
                MultiViewActivity.getStorageSpaceBytes() - Utils.LOW_STORAGE_THRESHOLD_BYTES;
        Log.d(TAG, "maximum file size is" + maxFileSize);
        Toast.makeText(mActivity, "maxFileSize is " + maxFileSize, Toast.LENGTH_SHORT).show();
        try {
            mMediaRecorder.setMaxFileSize(maxFileSize);
        } catch (RuntimeException exception) {
            // We are going to ignore failure of setMaxFileSize here, as
            // a) The composer selected may simply not support it, or
            // b) The underlying media framework may not handle 64-bit range
            // on the size restriction.
        }

        try {
            mMediaRecorder.prepare();
        } catch (IOException ex) {
            Log.e(TAG, "MediaRecorder prepare failed for " + mVideoFilename, ex);
            releaseMedia();
            throw new RuntimeException(ex);
        }

        mMediaRecorder.setOnErrorListener(this);
        mMediaRecorder.setOnInfoListener(this);
    }

    // from MediaRecorder.OnErrorListener
    @Override
    public void onError(MediaRecorder mr, int what, int extra) {
        Log.e(TAG, "MediaRecorder error. what=" + what + ". extra=" + extra);
        if (what == MediaRecorder.MEDIA_RECORDER_ERROR_UNKNOWN) {
            // We may have run out of space on the sdcard.
            stopRecordingVideo();
            MultiViewActivity.updateStorageSpace(null);
        }
    }

    // from MediaRecorder.OnInfoListener
    @Override
    public void onInfo(MediaRecorder mr, int what, int extra) {
        String[] VideofileDetails = new String[0];
        if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_APPROACHING) {
            Toast.makeText(mActivity, R.string.video_reach_size_limit, Toast.LENGTH_SHORT).show();

            if (mIsRecordingVideo) {
                saveVideo();

                VideofileDetails = Utils.generateFileDetails(Utils.MEDIA_TYPE_VIDEO);
                if (VideofileDetails == null || VideofileDetails.length < 5) {
                    Log.e(TAG, "setUpMediaRecorder Invalid file details");
                    return;
                }

                mVideoFilename = VideofileDetails[3];

                CamcorderProfile mProfile = getSelectedCamorderProfile();

                mCurrentVideoValues = Utils.getContentValues(
                        Utils.MEDIA_TYPE_VIDEO, VideofileDetails, mProfile.videoFrameWidth,
                        mProfile.videoFrameHeight, 0, 0);

                try {
                    mMediaRecorder.setNextOutputFile(new File(VideofileDetails[3]));
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        MultiViewActivity.updateStorageSpace(null);
    }

    private void closePreviewSession() {
        if (cameraCaptureSessions != null) {
            cameraCaptureSessions.close();
            cameraCaptureSessions = null;
        }
    }

    private void releaseMedia() {
        if (null != mMediaRecorder) {
            try {
                mMediaRecorder.setOnErrorListener(null);
                mMediaRecorder.setOnInfoListener(null);
                mMediaRecorder.stop();
                Log.i(TAG, "MediaRecorder  stop ");

            } catch (IllegalStateException e) {
                Log.e(TAG, "stop fail", e);
                if (mVideoFilename != null) {
                    deleteVideoFile(mVideoFilename);
                }
            }
            mMediaRecorder.reset();
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
    }

    private void deleteVideoFile(String fileName) {
        Log.v(TAG, "Deleting video " + fileName);
        File f = new File(fileName);
        if (!f.delete()) {
            Log.e(TAG, "Could not delete " + fileName);
        }
    }

    private void saveVideo() {
        long duration = SystemClock.uptimeMillis() - mRecordingStartTime;
        if (duration > 0) {
            //
        } else {
            Log.w(TAG, "Video duration <= 0 : " + duration);
        }
        Uri uri;

        mCurrentVideoValues.put(MediaStore.Video.Media.DURATION, duration);
        mCurrentVideoValues.put(MediaStore.Video.Media.SIZE, new File(mVideoFilename).length());

        uri = Utils.broadcastNewVideo(mActivity.getApplicationContext(), mCurrentVideoValues);

        mCurrentUri = uri;

        mCurrentFileInfo = mCurrentVideoValues;

        Log.i(TAG, "video saved @: " + mVideoFilename);

        mVideoFilename = null;
    }

    private void showVideoThumbnail() {
        mRoundedThumbnailView.startRevealThumbnailAnimation("Video taken");

        final Optional<Bitmap> bitmap =
                Utils.getVideoThumbnail(mActivity.getContentResolver(), mCurrentUri);

        if (bitmap.isPresent()) {
            mRoundedThumbnailView.setThumbnail(bitmap.get(), 0);
        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }

    private void VideoUI(boolean toggle) {
        if (toggle) {
            mRoundedThumbnailView.setVisibility(View.GONE);
            takePictureButton.setVisibility(View.GONE);
            SettingsView.setEnabled(false);
            SettingsView.setImageAlpha(95);
            showRecordingUI(mIsRecordingVideo);
            TakeVideoButton.setImageResource(R.drawable.ic_stop_normal);

        } else {
            showRecordingUI(mIsRecordingVideo);
            TakeVideoButton.setImageResource(R.drawable.ic_capture_video);
            takePictureButton.setVisibility(View.VISIBLE);
            SettingsView.setEnabled(true);
            SettingsView.setImageAlpha(200);
        }
    }

    private void stopRecordingVideo() {
        mHandler.removeMessages(MSG_UPDATE_RECORD_TIME);
        mIsRecordingVideo = false;

        releaseMedia();

        saveVideo();

        showVideoThumbnail();

        createCameraPreview();

        VideoUI(mIsRecordingVideo);
    }

    private void showDetailsDialog(ContentValues info) {
        Optional<MediaDetails> details = Utils.getMediaDetails(mActivity, info);
        if (!details.isPresent()) {
            return;
        }
        Dialog detailDialog = DetailsDialog.create(mActivity, details.get());
        detailDialog.show();
    }
}
