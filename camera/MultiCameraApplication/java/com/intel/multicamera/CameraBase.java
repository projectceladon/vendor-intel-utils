/*
 * Copyright (C) 2012 The Android Open Source Project
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
import android.graphics.Picture;
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
import android.widget.Button;
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
import java.util.concurrent.TimeUnit;


public class CameraBase  {
    private Activity mActivity;
    private String TAG;
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView textureView;
    private ImageButton FullScrn, SettingsView, takePictureButton, TakeVideoButton;

    private String cameraId;
    private CameraDevice mCameraDevice;
    private CameraCaptureSession cameraCaptureSessions;
    private CaptureRequest.Builder captureRequestBuilder;
    private Size previewSize;
    private ImageReader imageReader;
    private Handler mBackgroundHandler;
    private HandlerThread mBackgroundThread;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    private SharedPreferences settings;
    private SurfaceTexture mSurfaceTexture;
    private String Capture_Key, Video_key, SettingsKey;

    private CameraBase mCameraBase;
    private static final String SIZE_HD = "HD 720p";

    /**
     * Whether the app is recording video now
     */
    private boolean mIsRecordingVideo;
    private  MultiCamera ic_camera;

    private PhotoPreview mPhoto;
    private VideoRecord mRecord;
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

    private String[] ImageFileDetails;

    public CameraBase(Activity activity, AutoFitTextureView mtextureView, ImageButton[] Button,
                      TextView RecordingTimeView, String[] data,
                      RoundedThumbnailView roundedThumbnailView) {
        this.mActivity = activity;
        this.textureView = mtextureView;
        if (Button != null) {
            this.ClickListeners(Button[0], Button[1], Button[2], Button[4], Button[5]);
            SettingsView = Button[2];
            FullScrn = Button[3];
        }
        this.settings = PreferenceManager.getDefaultSharedPreferences(activity);
        cameraId = data[1];
        TAG = data[0];
        Capture_Key = data[2];
        Video_key = data[3];
        SettingsKey = data[4];

        ic_camera = MultiCamera.getInstance();

        mPhoto = new PhotoPreview(activity, roundedThumbnailView, cameraId);
        mRecord = new VideoRecord(this, Video_key,cameraId, mtextureView, mActivity,
            RecordingTimeView, SettingsKey);

        roundedThumbnailViewControlLayout = mActivity.findViewById(R.id.control1);
        mCameraBase = this;
    }

    private void ClickListeners(ImageButton PictureButton, ImageButton RecordButton,
        ImageButton Settings, ImageButton CameraSwitch, ImageButton CameraSplit) {
        TakePicureOnClicked(PictureButton);

        StartVideoRecording(RecordButton, PictureButton, CameraSwitch, CameraSplit, Settings);
        CameraSplit(CameraSplit);
    }


    private void TakePicureOnClicked(ImageButton PictureButton) {
        takePictureButton = PictureButton;
        if (takePictureButton == null) return;

        takePictureButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (ic_camera.getTopRightCam() != null &&
                        ic_camera.getTopRightCam() == mCameraBase) {
                    ic_camera.setOpenCameraId(1);
                }
                if (ic_camera.getBotLeftCam() != null && ic_camera.getBotLeftCam() == mCameraBase) {
                    ic_camera.setOpenCameraId(2);
                }
                if (ic_camera.getTopLeftCam() != null && ic_camera.getTopLeftCam() == mCameraBase) {
                    ic_camera.setOpenCameraId(0);
                }
                if (ic_camera.getBotRightCam() != null &&
                        ic_camera.getBotRightCam() == mCameraBase) {
                    ic_camera.setOpenCameraId(3);
                }
                System.out.println(TAG + "take pic start camera id"+ic_camera.getOpenCameraId());

                MultiViewActivity Mactivity = (MultiViewActivity) mActivity;
                Mactivity.closeCamera();
                ic_camera.setIsCameraOrSurveillance(0);
                Intent intent = new Intent(mActivity, SingleCameraActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
                mActivity.startActivity(intent);
                mActivity.finish();

            }
        });
    }

    private void CameraSplit(ImageButton cameraSplit) {
        if (cameraSplit == null) return;

        cameraSplit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Toast.makeText(mActivity, "camera split clicked ", Toast.LENGTH_LONG).show();
                System.out.println("camera split");
                MultiCamera ic_camera = MultiCamera.getInstance();
                MultiViewActivity Mactivity = (MultiViewActivity) mActivity;
                if (ic_camera.getIsCameraOrSurveillance() == 0) {
                    ic_camera.setIsCameraOrSurveillance(1);
                    Intent intent = new Intent(mActivity, MultiViewActivity.class);
                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    mActivity.startActivity(intent);
                    mActivity.finish();
                } else {
                    System.out.println("camera split calling single camera activity");
                    ic_camera.setIsCameraOrSurveillance(1);
                    Intent intent = new Intent(mActivity, SingleCameraActivity.class);
                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    mActivity.startActivity(intent);
                    mActivity.finish();
                }
            }
        });
    }

    private void StartVideoRecording(ImageButton RecordButton, final ImageButton Capture,
        ImageButton Switch, final ImageButton Split, final ImageButton Settings) {
        TakeVideoButton = RecordButton;

        TakeVideoButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                try {
                    MultiViewActivity MultiActivity = (MultiViewActivity) mActivity;

                    if (mIsRecordingVideo) {
                        mIsRecordingVideo = false;

                        mRecord.stopRecordingVideo();
                        mRecord.showRecordingUI(false);
                        mPhoto.showVideoThumbnail();

                        MultiActivity.enterFullScreen(view);
                        Capture.setVisibility(View.VISIBLE);
                        TakeVideoButton.setImageResource(R.drawable.ic_capture_video);
                        Split.setVisibility(View.VISIBLE);
                        Settings.setVisibility(View.VISIBLE);
                    } else {

                        mIsRecordingVideo =true;
                        mRecord.startRecordingVideo();
                        mRecord.showRecordingUI(true);
                        MultiActivity.enterFullScreen(view);
                        Settings.setVisibility(View.GONE);
                        Capture.setVisibility(View.GONE);
                        TakeVideoButton.setImageResource(R.drawable.ic_stop_normal);
                        Split.setVisibility(View.GONE);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Recording failed");
                }
            }
        });
    }

    TextureView.SurfaceTextureListener textureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            // open your camera here
            mSurfaceTexture = surface;
            try {
                TimeUnit.MILLISECONDS.sleep(100);
            } catch (Exception e) {

            }
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
            if (Key == null)
                return;

            previewSize = getSelectedDimension(Key);

            configureTransform(width, height);

            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);

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
            mCameraDevice = camera;
            mRecord.setCameraDevice(mCameraDevice);

            createCameraPreview();
            MultiViewActivity.updateStorageSpace(null);
        }

        @Override
        public void onDisconnected(CameraDevice camera) {
            Log.v(TAG, "onDisconnected");
            mCameraDevice = camera;
            closeCamera();
        }

        @Override
        public void onError(CameraDevice camera, int error) {
            Log.v(TAG, "onError");
            mCameraDevice = camera;
            closeCamera();
        }

        @Override
        public void onClosed(@NonNull CameraDevice camera) {
            Log.v(TAG, "onClose");
            mCameraDevice = camera;
            super.onClosed(camera);
            previewSize = SIZE_720P;
            configureTransform(textureView.getWidth(), textureView.getHeight());
            SurfaceUtil.clear(mSurfaceTexture);

            ResetResolutionSettings();
        }
    };

    private void ResetResolutionSettings() {
        SharedPreferences.Editor edit = settings.edit();
        edit.remove(Capture_Key);
        edit.remove(Video_key);
        edit.apply();
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
            mRecord.closePreviewSession();
            SurfaceTexture texture = textureView.getSurfaceTexture();
            if (texture == null) return;

            Surface surface = new Surface(texture);

            String Key = GetChnagedPrefKey();
            if (Key == null)
                return;

            previewSize = getSelectedDimension(Key);
            if (previewSize == null)
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
        try {
            if (mIsRecordingVideo) {
                mRecord.stopRecordingVideo();
            }
            mRecord.closePreviewSession();
            try {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        if (null != mCameraDevice) {
                            //mCameraDevice.wait(200);
                            mCameraDevice.close();
                            mCameraDevice = null;
                        }
                    }
                }).start();
            } catch (Exception e) {
                System.out.println(TAG +" camera close exception");
            }

            if (null != imageReader) {
                imageReader.close();
                imageReader = null;
            }

            mRecord.releaseMedia();

        } catch (Exception e) {
            System.out.println("close camera exception ");
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

    public void takePicture() {
        if (null == mCameraDevice) {
            Log.e(TAG, "cameraDevice is null");
            return;
        }

        try {
            final Size imageDimension = getSelectedDimension(Capture_Key);
            if (imageDimension == null) {
                Log.i(TAG, "Fail to get Dimension");
                return;
            }

            Log.i(TAG, "Still Capture imageDimension " + imageDimension.getWidth() + " x " +
                               imageDimension.getHeight());

            ImageReader reader = ImageReader.newInstance(
                    imageDimension.getWidth(), imageDimension.getHeight(), ImageFormat.JPEG, 1);
            List<Surface> outputSurfaces = new ArrayList<>(2);

            outputSurfaces.add(reader.getSurface());

            captureRequestBuilder =
                    mCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
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

                            mPhoto.showImageThumbnail(ImageFile);

                            createCameraPreview();
                        }
                    };
            mCameraDevice.createCaptureSession(
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

        ic_camera.setCurrentUri(uri);

        ic_camera.setCurrentFileInfo(mCurrentPictureValues);

        Log.i(TAG, "Image saved @ " + ImageFile.getAbsolutePath());
    }

    private void showDetailsDialog(ContentValues info) {
        Optional<MediaDetails> details = Utils.getMediaDetails(mActivity, info);
        if (!details.isPresent()) {
            return;
        }
        Dialog detailDialog = DetailsDialog.create(mActivity, details.get());
        detailDialog.show();
    }

    public PhotoPreview getmPhoto() {
        return mPhoto;
    }

    public VideoRecord getmRecord() {
        return mRecord;
    }

}
