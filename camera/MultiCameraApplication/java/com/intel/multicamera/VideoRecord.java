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
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.CamcorderProfile;
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
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.preference.PreferenceManager;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static com.intel.multicamera.SettingsPrefUtil.SIZE_HD;

public class VideoRecord implements MediaRecorder.OnErrorListener, MediaRecorder.OnInfoListener{

    private static final String TAG = "VideoRecord";
    private CameraDevice mCameraDevice;
    private AutoFitTextureView mTextureView;
    private Activity mActivity;
    private boolean mIsRecordingVideo;
    private CameraCaptureSession cameraCaptureSessions;
    private CaptureRequest.Builder captureRequestBuilder;
    private SharedPreferences settings;
    private MediaRecorder mMediaRecorder;
    private ContentValues mCurrentVideoValues;
    private String mVideoFilename, mCameraId;
    private int mSensorOrientation;
    private boolean mRecordingTimeCountsDown;
    private Handler mBackgroundHandler;
    private final Handler mHandler;
    private static final int SENSOR_ORIENTATION_DEFAULT_DEGREES = 90;
    private static final int SENSOR_ORIENTATION_INVERSE_DEGREES = 270;
    private static final int MSG_UPDATE_RECORD_TIME = 5;
    private static final SparseIntArray DEFAULT_ORIENTATIONS = new SparseIntArray();
    private static final SparseIntArray INVERSE_ORIENTATIONS = new SparseIntArray();
    private Size previewSize;
    private long mRecordingStartTime;
    private TextView mRecordingTimeView;
    private String mVideoKey, mSettingsKey;

    private CameraBase mCameraBase;
    private MultiCamera ic_camera;
    static final Size SIZE_720P = new Size(1280, 720);
    static final Size SIZE_480P = new Size(640, 480);

    public VideoRecord(CameraBase cameraBase, String videoKey, String cameraId,
            AutoFitTextureView textureView, Activity activity, TextView recordingView,
            String settingsKey) {
        mCameraBase = cameraBase;
        mTextureView = textureView;
        mActivity = activity;
        mVideoKey = videoKey;
        mCameraId = cameraId;
        mSettingsKey = settingsKey;
        previewSize = SIZE_720P;
        mRecordingTimeView  = recordingView;
        mRecordingTimeCountsDown = false;
        mHandler = new MainHandler();
        ic_camera = MultiCamera.getInstance();
    }
    /* Recording Start*/
    public void startRecordingVideo() {
        if (null == mCameraDevice || !mTextureView.isAvailable()) {
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

    public void stopRecordingVideo() {
        mHandler.removeMessages(MSG_UPDATE_RECORD_TIME);
        mIsRecordingVideo = false;

        releaseMedia();

        createCameraPreview();
        saveVideo();
    }


    private Size getSelectedDimension(String Key) {
        settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

        Size mDimensions = SIZE_720P;

        if (Key.compareTo(mVideoKey) == 0) {
            CamcorderProfile mProfile;

            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

            String videoQuality = settings.getString(mVideoKey, SIZE_HD);

            int quality = SettingsPrefUtil.getFromSetting(videoQuality);

            mProfile = CamcorderProfile.get(0, quality);

            mDimensions = new Size(mProfile.videoFrameWidth, mProfile.videoFrameHeight);

        }

        return mDimensions;
    }

    public void createCameraPreview() {
        try {

            closePreviewSession();

            SurfaceTexture texture = mTextureView.getSurfaceTexture();
            if (texture == null) return;

            Surface surface = new Surface(texture);

            String Key = GetChnagedPrefKey();

            previewSize = getSelectedDimension(Key);

            if(Key == null || previewSize == null)
                return;

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
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            captureRequestBuilder.addTarget(surface);

            mCameraDevice.createCaptureSession(
                    Arrays.asList(surface), new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(CameraCaptureSession cameraCaptureSession) {
                            // The camera is already closed
                            if (null == mCameraDevice) {
                                return;
                            }
                            // When the session is ready, we start displaying the preview.
                            cameraCaptureSessions = cameraCaptureSession;
                            updatePreview();
                        }

                        @Override
                        public void onConfigureFailed(CameraCaptureSession cameraCaptureSession) {
                            mCameraBase.closeCamera();
                            Toast.makeText(mActivity, " Preview Configuration change",
                                    Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void startRecorder(CamcorderProfile mProfile)
            throws IOException, CameraAccessException {
        closePreviewSession();

        SurfaceTexture texture = mTextureView.getSurfaceTexture();
        if (texture == null) return;

        if (mProfile.videoFrameWidth == 1280 || mProfile.videoFrameHeight == 720) {
            previewSize = SIZE_720P;

        } else if (mProfile.videoFrameWidth == 640 || mProfile.videoFrameHeight == 320) {
            previewSize = SIZE_480P;
        }

        Log.i(TAG,
                "Video previewSize  " + previewSize.getWidth() + " x " + previewSize.getHeight());

        texture.setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());

        adjustAspectRatio(previewSize.getWidth(), previewSize.getHeight());

        captureRequestBuilder = mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_RECORD);
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
        mCameraDevice.createCaptureSession(surfaces, new CameraCaptureSession.StateCallback() {
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


    public void closePreviewSession() {
        if (cameraCaptureSessions != null) {
            cameraCaptureSessions.close();
            cameraCaptureSessions = null;
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

    public void setRecordingViewTime(TextView RecordingTimeView) {

        mRecordingTimeView = RecordingTimeView;
    }

    private void setRecordingTimeTextColor(int color) {
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
        CameraManager manager = (CameraManager)mActivity.getSystemService(Context.CAMERA_SERVICE);
        try {
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(mCameraId);
            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
        } catch (Exception e) {

        }
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
            configureTransform(mTextureView.getWidth(), mTextureView.getHeight());
            return;
        }

        int viewWidth = mTextureView.getWidth();
        int viewHeight = mTextureView.getHeight();
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
        mTextureView.getTransform(txform);
        txform.setScale((float)newWidth / viewWidth, (float)newHeight / viewHeight);
        // txform.postRotate(10);          // just for fun
        txform.postTranslate(xoff, yoff);
        mTextureView.setTransform(txform);
    }

    private CamcorderProfile getSelectedCamorderProfile() {
        CamcorderProfile mProfile;

        settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
        String videoQuality = settings.getString(mVideoKey, SIZE_HD);

        int quality = SettingsPrefUtil.getFromSetting(videoQuality);

        Log.i(TAG, "Selected video quality " + videoQuality);

        mProfile = CamcorderProfile.get(0, quality);

        return mProfile;
    }


    public void releaseMedia() {
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


    private void updatePreview() {
        if (null == mCameraDevice) {
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


    private void configureTransform(int viewWidth, int viewHeight) {
        if (null == mTextureView || null == previewSize) {
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
        mTextureView.setTransform(matrix);
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


    private String GetChnagedPrefKey() {
        String Key = null;

        switch (mSettingsKey) {
            case "pref_resolution":
                Key = getString(mSettingsKey, "capture_list");
                break;
            case "pref_resolution_1":
                Key = getString(mSettingsKey, "capture_list_1");
                break;
            case "pref_resolution_2":
                Key = getString(mSettingsKey, "capture_list_2");
                break;
            case "pref_resolution_3":
                Key = getString(mSettingsKey, "capture_list_3");
                break;
            default:
                break;
        }

        return Key;
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

    private void saveVideo() {
        long duration = SystemClock.uptimeMillis() - mRecordingStartTime;
        Log.w(TAG, "Video duration <= 0 : " + duration);
        Uri uri;

        mCurrentVideoValues.put(MediaStore.Video.Media.DURATION, duration);
        mCurrentVideoValues.put(MediaStore.Video.Media.SIZE, new File(mVideoFilename).length());

        uri = Utils.broadcastNewVideo(mActivity.getApplicationContext(), mCurrentVideoValues);

        ic_camera.setCurrentUri(uri);

        ic_camera.setCurrentFileInfo(mCurrentVideoValues);

        Log.i(TAG, "video saved @: " + mVideoFilename);

        mVideoFilename = null;
    }

    void setCameraDevice(CameraDevice cameraDevice) {
        mCameraDevice = cameraDevice;
    }
}
