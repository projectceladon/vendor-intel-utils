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

public class TopRightCam {
    Activity mActivity;
    private static final String TAG = "TopRightCam";
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
    private Size imageDimension;
    private ImageReader imageReader;
    private File file;
    private static final int REQUEST_CAMERA_PERMISSION = 200;
    private final int PERMISSIONS_REQUEST_SNAPSHOT = 3;

    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;
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

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    public TopRightCam(Activity activity, AutoFitTextureView textureView, Button PictureButton,
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
            openCamera();
        }
        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            // Transform you image captured size according to the surface width and height
        }
        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            return false;
        }
        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {}
    };

    public void openCamera() {
        CameraManager manager = (CameraManager)mActivity.getSystemService(Context.CAMERA_SERVICE);
        Log.e(TAG, "is camera open");
        try {
            cameraId = manager.getCameraIdList()[1];
            Log.e(TAG, "is camera open ID" + cameraId);
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map =
                characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) return;

            // Add permission for camera and let user grant the permission
            if (ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.CAMERA) !=
                    PackageManager.PERMISSION_GRANTED &&
                ActivityCompat.checkSelfPermission(mActivity,
                                                   Manifest.permission.WRITE_EXTERNAL_STORAGE) !=
                    PackageManager.PERMISSION_GRANTED &&
                ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.RECORD_AUDIO) !=
                    PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                    mActivity,
                    new String[] {Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO,
                                  Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_CAMERA_PERMISSION);
                return;
            }

            mMediaRecorder = new MediaRecorder();

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
            cameraDevice.close();
            cameraDevice = null;
        }
    };

    protected void createCameraPreview() {
        try {
            closePreviewSession();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;

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
        if (null != mMediaRecorder) {
            mMediaRecorder.reset();
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
        stopBackgroundThread();
    }

    /**
     * Starts a background thread and its {@link Handler}.
     */
    private void startBackgroundThread() {
        mBackgroundThread = new HandlerThread("Camera-$cameraId");
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

    protected void takePicture() {
        if (null == cameraDevice) {
            Log.e(TAG, "cameraDevice is null");
            return;
        }

        try {
            imageDimension = SettingsActivity.SettingsFragment.sizeFromSettingString(
                settings.getString("capture_list", "640x480"));

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
            captureRequestBuilder.set(CaptureRequest.JPEG_ORIENTATION, ORIENTATIONS.get(rotation));

            // Add permission for camera and let user grant the permission
            if (ActivityCompat.checkSelfPermission(mActivity,
                                                   Manifest.permission.WRITE_EXTERNAL_STORAGE) !=
                PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(
                    mActivity, new String[] {Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    PERMISSIONS_REQUEST_SNAPSHOT);
                return;
            }

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
                    public void onConfigureFailed(CameraCaptureSession session) {}
                }, mBackgroundHandler);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    /* Recording Start*/
    private void startRecordingVideo() {
        if (null == cameraDevice || !textureView.isAvailable() || null == mProfile) {
            return;
        }
        try {
            closePreviewSession();
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
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.CAMCORDER);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.SURFACE);
        mMediaRecorder.setProfile(mProfile);

        String fileDetails[] = Utils.generateFileDetails(Utils.MEDIA_TYPE_VIDEO);
        if (fileDetails == null || fileDetails.length < 5) {
            Log.e(TAG, "Invalid file details");
            return;
        }
        mVideoFilename = fileDetails[3];
        mCurrentVideoValues =
            Utils.getContentValues(Utils.MEDIA_TYPE_VIDEO, fileDetails, mProfile.videoFrameWidth,
                                   mProfile.videoFrameHeight);

        /**
         * set output file in media recorder
         */
        mMediaRecorder.setOutputFile(mVideoFilename);

        try {
            mMediaRecorder.prepare();
        } catch (IOException ex) {
            Log.e(TAG, "prepare failed for " + mVideoFilename, ex);
            mMediaRecorder.reset();
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
        mMediaRecorder.stop();
        mMediaRecorder.reset();

        if (null != mActivity) {
            Toast.makeText(mActivity, "Video saved: " + mVideoFilename, Toast.LENGTH_SHORT).show();
            Log.d(TAG, "Video saved: " + mVideoFilename);
        }
        mVideoFilename = null;

        createCameraPreview();
    }
}
