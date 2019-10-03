/*
 * Copyright 2014 The Android Open Source Project
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
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.CamcorderProfile;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.provider.MediaStore;
import android.util.Log;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.preference.PreferenceManager;
import java.io.*;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class BotmRightCam {
    Activity mActivity;
    private static final String TAG = "BotmRightCam";
    private String mNextVideoAbsolutePath;
    private CamcorderProfile mProfile;
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView textureView;
    private Button takePictureButton, TakeVideoButton;

    private MediaRecorder mMediaRecorder;
    private String cameraId;
    protected CameraDevice cameraDevice;
    protected CameraCaptureSession cameraCaptureSessions;
    protected CaptureRequest captureRequest;
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
    /**
     * Whether the app is recording video now
     */
    private boolean mIsRecordingVideo;

    // The video file that the hardware camera is about to record into
    // (or is recording into.
    private String mVideoFilename, mPictureFilename;
    private ContentValues mCurrentVideoValues, mCurrentPictureValues;
    byte[] jpegLength;

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

    public BotmRightCam(Activity activity, AutoFitTextureView textureView, Button PictureButton,
                        Button RecordButton) {
        Log.e(TAG, "constructor called");
        this.mActivity = activity;
        this.textureView = textureView;
        this.textureView.setSurfaceTextureListener(textureListener);
        this.ClickListeners(PictureButton, RecordButton);
        this.settings = PreferenceManager.getDefaultSharedPreferences(activity);
    }

    public void ClickListeners(Button PictureButton, Button RecordButton) {
        TakePicureOnClicked(PictureButton);

        StartVideoRecording(RecordButton);
    }

    private void TakePicureOnClicked(Button PictureButton) {
        takePictureButton = PictureButton;
        if (takePictureButton == null) return;

        takePictureButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                takePicture();
                Utils.broadcastNewPicture(mActivity.getApplicationContext(), mCurrentPictureValues);
            }
        });
    }

    private void StartVideoRecording(Button RecordButton) {
        TakeVideoButton = RecordButton;
        TakeVideoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // Intent i = new Intent(HomeScreenActivity.this, CameraActivity.class);
                System.out.println(" onCreate Record0");
                if (mIsRecordingVideo) {
                    stopRecordingVideo();
                    Utils.broadcastNewVideo(mActivity.getApplicationContext(), mCurrentVideoValues);
                    takePictureButton.setVisibility(View.VISIBLE);
                } else {
                    startRecordingVideo();
                    takePictureButton.setVisibility(View.GONE);
                }
            }
        });
    }

    TextureView.SurfaceTextureListener textureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            // open your camera here
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
        Log.e(TAG, "is camera open");
        try {
            if (manager.getCameraIdList().length != 4) {
                Log.e(TAG, "this camera is not connected ");
                return;
            }

            cameraId = manager.getCameraIdList()[3];
            Log.e(TAG, "is camera open ID" + cameraId);
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map =
                    characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) return;
            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);
            String Key = SettingsActivity.SettingsFragment.getchangedPrefKey();

            if (Key.compareTo("video_list") == 0) {
                String videoQuality = settings.getString("video_list", "medium");

                int quality = SettingsActivity.SettingsFragment.getVideoQuality(0, videoQuality);
                Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);

                mProfile = CamcorderProfile.get(0, quality);
                previewSize = new Size(mProfile.videoFrameWidth, mProfile.videoFrameHeight);

                configureTransform(width, height);
            } else {
                previewSize = SettingsActivity.SettingsFragment.sizeFromSettingString(
                        settings.getString("capture_list", "640x480"));
                Log.d(TAG,
                      "Selected imageDimension" + previewSize.getWidth() + previewSize.getHeight());
                configureTransform(width, height);
            }
            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
            configureTransform(width, height);
            startBackgroundThread();

            manager.openCamera(cameraId, stateCallback, null);

        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
        Log.e(TAG, "openCamera X");
    }

    private final CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            // This is called when the camera is open
            Log.e(TAG, "onOpened");
            cameraDevice = camera;
            createCameraPreview();
        }
        @Override
        public void onDisconnected(CameraDevice camera) {
            Log.e(TAG, "onDisconnected");
            closeCamera();
        }
        @Override
        public void onError(CameraDevice camera, int error) {
            Log.e(TAG, "onError");
            closeCamera();
        }
    };

    private void configureTransform(int viewWidth, int viewHeight) {
        if (null == textureView || null == previewSize) {
            return;
        }
        int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        Matrix matrix = new Matrix();
        RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
        Log.e(TAG, "configureTransform() viewWidth: " + viewWidth + " viewHeight: " + viewHeight);
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

    protected void createCameraPreview() {
        try {
            closePreviewSession();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;
            settings = PreferenceManager.getDefaultSharedPreferences(mActivity);

            String Key = SettingsActivity.SettingsFragment.getchangedPrefKey();
            imageDimension = SettingsActivity.SettingsFragment.sizeFromSettingString(
                    settings.getString("capture_list", "640x480"));

            String videoQuality = settings.getString("video_list", "medium");
            int quality = SettingsActivity.SettingsFragment.getVideoQuality(0, videoQuality);
            Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);

            mProfile = CamcorderProfile.get(0, quality);

            if (Key.compareTo("video_list") == 0) {
                texture.setDefaultBufferSize(mProfile.videoFrameWidth, mProfile.videoFrameHeight);
            } else {
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
                            Toast.makeText(mActivity, "Configuration change", Toast.LENGTH_SHORT)
                                    .show();
                        }
                    }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
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
        mBackgroundThread = new HandlerThread("Camera-4");
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
            imageDimension = SettingsActivity.SettingsFragment.sizeFromSettingString(
                    settings.getString("capture_list", "640x480"));
            Log.d(TAG, "Selected imageDimension" + imageDimension.getWidth() +
                               imageDimension.getHeight());
            ImageReader reader = ImageReader.newInstance(
                    imageDimension.getWidth(), imageDimension.getHeight(), ImageFormat.JPEG, 1);
            List<Surface> outputSurfaces = new ArrayList<Surface>(2);
            outputSurfaces.add(reader.getSurface());
            outputSurfaces.add(new Surface(textureView.getSurfaceTexture()));
            captureRequestBuilder =
                    cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureRequestBuilder.addTarget(reader.getSurface());
            captureRequestBuilder.set(CaptureRequest.CONTROL_MODE,
                                      CameraMetadata.CONTROL_MODE_AUTO);
            // Orientation
            int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
            captureRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, getOrientation(rotation));

            String fileDetails[] = Utils.generateFileDetails(Utils.MEDIA_TYPE_IMAGE);
            if (fileDetails == null || fileDetails.length < 5) {
                Log.e(TAG, "Invalid file details");
                return;
            }
            mPictureFilename = fileDetails[3];
            mCurrentPictureValues =
                    Utils.getContentValues(Utils.MEDIA_TYPE_IMAGE, fileDetails,
                                           imageDimension.getWidth(), imageDimension.getHeight());

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
                                jpegLength = bytes;
                                mCurrentPictureValues.put(MediaStore.Images.ImageColumns.SIZE,
                                                          jpegLength);

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
                            Toast.makeText(mActivity, "Saved:" + file, Toast.LENGTH_SHORT).show();

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
            String videoQuality = settings.getString("video_list", "medium");

            int quality = SettingsActivity.SettingsFragment.getVideoQuality(0, videoQuality);
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
                            TakeVideoButton.setText(R.string.stop);
                            mIsRecordingVideo = true;

                            // Start recording
                            mMediaRecorder.start();
                        }
                    });
                }

                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                    if (null != mActivity) {
                        Toast.makeText(mActivity, "Failed", Toast.LENGTH_SHORT).show();
                    }
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

        String fileDetails[] = Utils.generateFileDetails(Utils.MEDIA_TYPE_VIDEO);
        if (fileDetails == null || fileDetails.length < 5) {
            Log.e(TAG, "Invalid file details");
            return;
        }
        mVideoFilename = fileDetails[3];
        mCurrentVideoValues =
                Utils.getContentValues(Utils.MEDIA_TYPE_VIDEO, fileDetails,
                                       mProfile.videoFrameWidth, mProfile.videoFrameHeight);

        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
        /**
         * set output file in media recorder
         */
        mMediaRecorder.setOutputFile(mVideoFilename);
        mMediaRecorder.setVideoEncodingBitRate(10000000);
        mMediaRecorder.setVideoFrameRate(30);
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

    private void stopRecordingVideo() {
        mIsRecordingVideo = false;
        TakeVideoButton.setText(R.string.record);

        // Stop recording
        releaseMedia();

        if (null != mActivity) {
            Toast.makeText(mActivity, "Video saved: " + mVideoFilename, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "Video saved: " + mVideoFilename);
        }
        mVideoFilename = null;

        createCameraPreview();
    }
}
