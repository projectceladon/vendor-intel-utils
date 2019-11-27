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
import android.content.ContentValues;
import android.content.Context;
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

public class CameraBase {
    Activity mActivity;
    private String TAG;
    private CamcorderProfile mProfile;
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView textureView;
    private ImageButton FullScrn, SettingsView, takePictureButton, TakeVideoButton;

    private MediaRecorder mMediaRecorder;
    private String cameraId;
    protected CameraDevice cameraDevice;
    protected CameraCaptureSession cameraCaptureSessions;
    protected CaptureRequest.Builder captureRequestBuilder;
    private Size imageDimension, previewSize;
    private ImageReader imageReader;
    private File file;
    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;
    private static final int SENSOR_ORIENTATION_DEFAULT_DEGREES = 90;
    private static final int SENSOR_ORIENTATION_INVERSE_DEGREES = 270;
    private static final SparseIntArray DEFAULT_ORIENTATIONS = new SparseIntArray();
    private static final SparseIntArray INVERSE_ORIENTATIONS = new SparseIntArray();
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    private SharedPreferences settings;
    private FrameLayout frameView0;
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
    private String mVideoFilename, mPictureFilename;
    private ContentValues mCurrentVideoValues, mCurrentPictureValues;
    byte[] jpegLength;
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

    private RoundedThumbnailView mRoundedThumbnailView;
    FrameLayout roundedThumbnailViewControlLayout;

    private Uri mCurrentUri;
    private String[] VideofileDetails;

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

        Log.e(TAG, "constructor called");
    }

    private void RoundedThumbnail_setOnClickListners() {
        mRoundedThumbnailView.setCallback(new RoundedThumbnailView.Callback() {
            ImageView preView;

            @Override
            public void onHitStateFinished() {
                ImageView preView;
                ImageButton btnDelete, playButton, btnBack;
                FrameLayout previewLayout;

                String mimeType = Utils.getMimeTypeFromURI(mActivity, mCurrentUri);

                previewLayout = mActivity.findViewById(R.id.previewLayout);
                previewLayout.setVisibility(View.VISIBLE);

                btnDelete = mActivity.findViewById(R.id.control_delete);
                playButton = mActivity.findViewById(R.id.play_button);
                preView = mActivity.findViewById(R.id.preview);
                btnBack = mActivity.findViewById(R.id.control_back);

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
                            Log.e(TAG, " File Deleted ");
                            file.delete();
                            preView.setImageResource(android.R.color.background_dark);
                        }
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

        playButton.setVisibility(View.VISIBLE);

        playButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Utils.playVideo(mActivity, mCurrentUri, TAG);
            }
        });

        preView.setVisibility(View.VISIBLE);
        preView.setImageBitmap(bitmap.get());
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

    public void setRecordingTime(String text) {
        mRecordingTimeView.setText(text);
    }

    public void setRecordingTimeTextColor(int color) {
        mRecordingTimeView.setTextColor(color);
    }

    public void showRecordingUI(boolean recording) {
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

    public void ClickListeners(ImageButton PictureButton, ImageButton RecordButton) {
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
                    showRecordingUI(mIsRecordingVideo);
                    TakeVideoButton.setImageResource(R.drawable.ic_capture_video);
                    takePictureButton.setVisibility(View.VISIBLE);
                    SettingsView.setEnabled(true);
                    SettingsView.setImageAlpha(200);

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
            configureTransform(width, height);
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            return false;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        }
    };

    public void openCamera(int width, int height) {
        CameraManager manager = (CameraManager)mActivity.getSystemService(Context.CAMERA_SERVICE);
        try {
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) return;
            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

            String Key = GetChnagedPrefKey();

            if (Key.compareTo(Capture_Key) == 0) {
                previewSize = SettingsPrefUtil.sizeFromSettingString(
                        settings.getString(Capture_Key, "1280x720"));
                Log.d(TAG,
                      "Selected imageDimension" + previewSize.getWidth() + previewSize.getHeight());
            } else {
                String videoQuality = settings.getString(Video_key, SIZE_HD);

                int quality = SettingsPrefUtil.getFromSetting(videoQuality);
                Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);

                mProfile = CamcorderProfile.get(0, quality);
                previewSize = new Size(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
            }

            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
            configureTransform(width, height);
            startBackgroundThread();

            manager.openCamera(cameraId, stateCallback, null);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        Log.e(TAG, "openCamera");
    }

    private final CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            // This is called when the camera is open
            Log.e(TAG, "onOpened");
            cameraDevice = camera;
            Ui_Enable(true);
            createCameraPreview();
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            Log.e(TAG, "onDisconnected");
            closeCamera();
            Ui_Enable(false);
        }

        @Override
        public void onError(CameraDevice camera, int error) {
            Log.e(TAG, "onError");
            closeCamera();
            Ui_Enable(false);
        }

        @Override
        public void onClosed(@NonNull CameraDevice camera) {
            Log.e(TAG, "onClose");
            super.onClosed(camera);
            SurfaceUtil.clear(mSurfaceTexture);
            Ui_Enable(false);
            ResetResolutionSettings();
        }
    };

    private void ResetResolutionSettings() {
        SharedPreferences.Editor edit = settings.edit();
        edit.remove(Capture_Key);
        edit.remove(Video_key);
        edit.commit();
    }

    private void Ui_Enable(boolean flag) {
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
        Log.e(TAG, "configureTransform() viewWidth: " + viewWidth + " viewHeight: " + viewHeight +
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
    public String getString(String key, String defaultValue) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(mActivity);
        try {
            return preferences.getString(key, defaultValue);
        } catch (ClassCastException e) {
            Log.w(TAG, "existing preference with invalid type,removing and returning default", e);
            preferences.edit().remove(key).apply();
            return defaultValue;
        }
    }

    public String GetChnagedPrefKey() {
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

    protected void createCameraPreview() {
        try {
            closePreviewSession();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;

            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

            String Key = GetChnagedPrefKey();

            imageDimension = SettingsPrefUtil.sizeFromSettingString(
                    settings.getString(Capture_Key, "1280x720"));
            String videoQuality = settings.getString(Video_key, SIZE_HD);

            int quality = SettingsPrefUtil.getFromSetting(videoQuality);
            Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);

            mProfile = CamcorderProfile.get(0, quality);

            if (Key.compareTo(Video_key) == 0) {
                previewSize = new Size(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
                texture.setDefaultBufferSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
            } else {
                previewSize = imageDimension;
                texture.setDefaultBufferSize(imageDimension.getWidth(), imageDimension.getHeight());
            }

            Surface surface = new Surface(texture);
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
                            Toast.makeText(mActivity, "Configuration change", Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void closeCamera() {
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
        mBackgroundThread = new HandlerThread("Camera_1");
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

    protected void updatePreview() {
        if (null == cameraDevice) {
            Log.e(TAG, "updatePreview error, return");
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

    protected void takePicture() {
        if (null == cameraDevice) {
            Log.e(TAG, "cameraDevice is null");
            return;
        }

        try {
            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
            imageDimension = SettingsPrefUtil.sizeFromSettingString(
                    settings.getString(Capture_Key, "1280x720"));
            Log.d(TAG, "Selected imageDimension" + imageDimension.getWidth() +
                               imageDimension.getHeight());
            ImageReader reader = ImageReader.newInstance(
                    imageDimension.getWidth(), imageDimension.getHeight(), ImageFormat.JPEG, 1);
            List<Surface> outputSurfaces = new ArrayList<Surface>(2);
            outputSurfaces.add(reader.getSurface());
            outputSurfaces.add(new Surface(textureView.getSurfaceTexture()));

            textureView.getSurfaceTexture().setDefaultBufferSize(imageDimension.getWidth(),
                                                                 imageDimension.getHeight());

            captureRequestBuilder =
                    cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureRequestBuilder.addTarget(reader.getSurface());
            captureRequestBuilder.set(CaptureRequest.CONTROL_MODE,
                                      CameraMetadata.CONTROL_MODE_AUTO);
            // Orientation
            int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
            final int mRotation = rotation;
            captureRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, getOrientation(rotation));

            final String fileDetails[] = Utils.generateFileDetails(Utils.MEDIA_TYPE_IMAGE);
            if (fileDetails == null || fileDetails.length < 5) {
                Log.e(TAG, "Invalid file details");
                return;
            }
            mPictureFilename = fileDetails[3];

            file = new File(mPictureFilename);

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
                                output = new FileOutputStream(file);
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

                            mCurrentPictureValues = Utils.getContentValues(
                                    Utils.MEDIA_TYPE_IMAGE, fileDetails, imageDimension.getWidth(),
                                    imageDimension.getHeight(), 0, file.length());

                            Utils.broadcastNewPicture(mActivity.getApplicationContext(),
                                                      mCurrentPictureValues);

                            mCurrentUri = Utils.getCurrentPictureUri();

                            final Optional<Bitmap> bitmap = Utils.generateThumbnail(
                                    file, roundedThumbnailViewControlLayout.getWidth(),
                                    roundedThumbnailViewControlLayout.getMeasuredHeight());

                            if (bitmap.isPresent()) {
                                mActivity.runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        mRoundedThumbnailView.startRevealThumbnailAnimation(
                                                "photo taken");
                                        mRoundedThumbnailView.setThumbnail(
                                                bitmap.get(), getOrientation(mRotation));
                                    }
                                });

                            } else {
                                Log.e(TAG, "No bitmap image found: ");
                            }

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

    /* Recording Start*/
    private void startRecordingVideo() {
        if (null == cameraDevice || !textureView.isAvailable()) {
            return;
        }
        try {
            closePreviewSession();
            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
            String videoQuality = settings.getString(Video_key, SIZE_HD);

            int quality = SettingsPrefUtil.getFromSetting(videoQuality);
            Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);

            mProfile = CamcorderProfile.get(0, quality);
            setUpMediaRecorder();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;
            texture.setDefaultBufferSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);

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
                            mRecordingStartTime = SystemClock.uptimeMillis();
                            takePictureButton.setVisibility(View.GONE);
                            SettingsView.setEnabled(false);
                            SettingsView.setImageAlpha(95);
                            showRecordingUI(mIsRecordingVideo);
                            TakeVideoButton.setImageResource(R.drawable.ic_stop_normal);
                            updateRecordingTime();
                        }
                    });
                }

                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                    if (null != mActivity) {
                        Toast.makeText(mActivity, "Recording Failed", Toast.LENGTH_SHORT).show();
                    }

                    releaseMedia();
                }
            }, mBackgroundHandler);
        } catch (CameraAccessException | IOException e) {
            e.printStackTrace();
        }
    }

    private void setUpMediaRecorder() throws IOException {
        if (null == mActivity) {
            return;
        }

        mMediaRecorder = new MediaRecorder();
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);

        {
            Context mContext = mActivity.getApplicationContext();
            AudioManager mAudioManager =
                    (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
            AudioDeviceInfo[] deviceList =
                    mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
            for (int index = 0; index < deviceList.length; index++) {
                if (deviceList[index].getType() == AudioDeviceInfo.TYPE_USB_DEVICE) {
                    mMediaRecorder.setPreferredDevice(deviceList[index]);
                    break;
                }
            }
        }

        VideofileDetails = Utils.generateFileDetails(Utils.MEDIA_TYPE_VIDEO);
        if (VideofileDetails == null || VideofileDetails.length < 5) {
            Log.e(TAG, "Invalid file details");
            return;
        }

        mVideoFilename = VideofileDetails[3];
        file = new File(mVideoFilename);

        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        /**
         * set output file in media recorder
         */
        mMediaRecorder.setOutputFile(mVideoFilename);

        mMediaRecorder.setVideoEncodingBitRate(10000000);
        mMediaRecorder.setVideoFrameRate(mProfile.videoFrameRate);
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
        try {
            mMediaRecorder.prepare();
        } catch (IOException ex) {
            Log.e(TAG, "prepare failed for " + mVideoFilename, ex);
            releaseMedia();
            throw new RuntimeException(ex);
        }
    }

    private void closePreviewSession() {
        System.out.println(" closePreviewSession");
        if (cameraCaptureSessions != null) {
            cameraCaptureSessions.close();
            cameraCaptureSessions = null;
        }
    }

    public void releaseMedia() {
        if (null != mMediaRecorder) {
            try {
                mMediaRecorder.stop();
            } catch (IllegalStateException ex) {
                Log.d(TAG, "Stop called before start");
            }
            mMediaRecorder.reset();
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
    }

    private void saveVideo() {
        long duration = SystemClock.uptimeMillis() - mRecordingStartTime;
        if (duration > 0) {
            //
        } else {
            Log.w(TAG, "Video duration <= 0 : " + duration);
        }

        mCurrentVideoValues = Utils.getContentValues(
                Utils.MEDIA_TYPE_VIDEO, VideofileDetails, mProfile.videoFrameWidth,
                mProfile.videoFrameHeight, duration, new File(mVideoFilename).length());

        Utils.broadcastNewVideo(mActivity.getApplicationContext(), mCurrentVideoValues);
    }

    private void stopRecordingVideo() {
        mHandler.removeMessages(MSG_UPDATE_RECORD_TIME);
        mIsRecordingVideo = false;

        releaseMedia();

        saveVideo();

        mCurrentUri = Utils.getCurrentVideoUri();

        mRoundedThumbnailView.startRevealThumbnailAnimation("Video taken");

        final Optional<Bitmap> bitmap =
                Utils.getVideoThumbnail(mActivity.getContentResolver(), mCurrentUri);

        if (bitmap.isPresent()) {
            mRoundedThumbnailView.setThumbnail(bitmap.get(), 0);
        } else {
            Log.e(TAG, "No bitmap image found: ");
        }

        mVideoFilename = null;

        createCameraPreview();
    }
}
