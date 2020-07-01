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

import android.Manifest;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import java.util.Optional;
import java.util.concurrent.TimeUnit;

import static com.intel.multicamera.MultiViewActivity.updateStorageSpace;

public class SingleCameraActivity extends AppCompatActivity {
    private static final String TAG = "SingleCameraActivity";
    private boolean mHasCriticalPermissions;
    private int isDialogShown;
    private int numOfCameras;
    private AutoFitTextureView mCamera_BackView, mCamera_FrontView;

    private ImageButton mCameraSwitch, mCameraPicture, mCameraRecord, mCameraSplit, mSettings;
    public String[] CameraIds;
    MultiCamera mCameraInst;
    private TextView mRecordingTimeView;
    private boolean mIsRecordingVideo;
    private CameraBase mCamera;
    private RoundedThumbnailView mRoundedThumbnailView;
    private BroadcastReceiver mUsbReceiver;
    private long mLastClickTime = 0;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
        }
        isDialogShown = 0;
        setContentView(R.layout.activity_full_screen);

        mIsRecordingVideo = false;
        mCameraInst = MultiCamera.getInstance();
        mCameraSwitch = findViewById(R.id.camera_switch);
        mCameraSwitch.setVisibility(View.VISIBLE);
        if (mCameraInst.getOpenCameraId() == 2 || mCameraInst.getOpenCameraId() == 3)
            mCameraSwitch.setVisibility(View.GONE);
        mCameraPicture = findViewById(R.id.Picture);
        mCameraRecord = findViewById(R.id.Record);
        mCameraSplit = findViewById(R.id.camera_split_view);
        mRecordingTimeView = findViewById(R.id.recording_time);
        mSettings = findViewById(R.id.SettingView);
        mRoundedThumbnailView = findViewById(R.id.rounded_thumbnail_view);

        checkPermissions();
        OpenCamera();
        try {
            IntentFilter filter = new IntentFilter();
            filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
            filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);

            // BroadcastReceiver when insert/remove the device USB plug into/from a USB port
            mUsbReceiver = new BroadcastReceiver() {
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                        System.out.println(TAG + "BroadcastReceiver USB Connected");
                        if (isDialogShown == 0) {
                            Dialog detailDialog = USBChangeDialog.create(SingleCameraActivity.this);
                            detailDialog.setCancelable(false);
                            detailDialog.setCanceledOnTouchOutside(false);
                            detailDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                            detailDialog.show();
                            isDialogShown = 1;
                            mCameraSwitch.setVisibility(View.GONE);
                            mCameraRecord.setVisibility(View.GONE);
                            mCameraPicture.setVisibility(View.GONE);
                            mCameraSplit.setVisibility(View.GONE);
                        }

                    } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                        System.out.println(TAG + "BroadcastReceiver USB Disconnected");
                        if (isDialogShown == 0) {
                            Dialog detailDialog = USBChangeDialog.create(SingleCameraActivity.this);
                            detailDialog.setCancelable(false);
                            detailDialog.setCanceledOnTouchOutside(false);
                            detailDialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
                            detailDialog.show();
                            isDialogShown = 1;
                            mCameraSwitch.setVisibility(View.GONE);
                            mCameraRecord.setVisibility(View.GONE);
                            mCameraPicture.setVisibility(View.GONE);
                            mCameraSplit.setVisibility(View.GONE);
                        }
                    }
                }
            };
            registerReceiver(mUsbReceiver , filter);
        } catch (Exception e) {
            //there is race condition when camera connection app is trying to open during that
            // time is camera is disconnect suddently then its trying to access non exist device
            //nodes hence crash is observed
            System.out.println(TAG + " Exception occured during USB attach and detach");
        }

        mCameraSwitch.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (SystemClock.elapsedRealtime() - mLastClickTime < 1000) {
                    return;
                }
                mLastClickTime = SystemClock.elapsedRealtime();
                MultiCamera ic_camera = MultiCamera.getInstance();
                if (ic_camera.getWhichCamera() == 0) {
                    ic_camera.setOpenCameraId(1);
                    OpenCamera();
                    ic_camera.setWhichCamera(1);
                    Log.i(TAG,"Opened front camera");
                }
                else {
                    ic_camera.setOpenCameraId(0);
                    OpenCamera();
                    ic_camera.setWhichCamera(0);
                    Log.i(TAG,"Opened back camera");
                }
            }
        });

        mCameraSplit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MultiCamera ic_camera = MultiCamera.getInstance();
                ic_camera.setIsCameraOrSurveillance(1);
                Intent intent = new Intent(SingleCameraActivity.this, MultiViewActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(intent);
                finish();

            }
        });

        mCameraPicture.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (SystemClock.elapsedRealtime() - mLastClickTime < 1000) {
                    return;
                }
                mLastClickTime = SystemClock.elapsedRealtime();
                mCamera.takePicture();

            }
        });

        mCameraRecord.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (SystemClock.elapsedRealtime() - mLastClickTime < 1000) {
                    return;
                }
                mLastClickTime = SystemClock.elapsedRealtime();

                try {
                    if (mIsRecordingVideo) {
                        mIsRecordingVideo = false;

                        mCamera.getmRecord().stopRecordingVideo();
                        mCamera.getmRecord().showRecordingUI(false);
                        mCamera.getmPhoto().showVideoThumbnail();

                        mCameraSwitch.setVisibility(View.VISIBLE);
                        mCameraPicture.setVisibility(View.VISIBLE);
                        mCameraRecord.setImageResource(R.drawable.ic_capture_video);
                        mCameraSplit.setVisibility(View.VISIBLE);
                        mSettings.setVisibility(View.VISIBLE);
                    } else {
                        mIsRecordingVideo =true;

                        mCamera.getmRecord().startRecordingVideo();

                        mCamera.getmRecord().showRecordingUI(true);
                        mSettings.setVisibility(View.GONE);
                        mCameraSwitch.setVisibility(View.GONE);
                        mCameraPicture.setVisibility(View.GONE);
                        mCameraRecord.setImageResource(R.drawable.ic_stop_normal);
                        mCameraSplit.setVisibility(View.GONE);
                    }
                } catch (Exception e) {
                    System.out.println(TAG + "exception during record");
                }
            }
        });
    }

    public void OpenCamera() {
        closeCamera();

        GetCameraCnt();
        if (numOfCameras == 0) {
            Toast.makeText(SingleCameraActivity.this, "No Camera Found", Toast.LENGTH_LONG).show();
            System.out.println(TAG+" no camera found");
            return;
        }
        if (numOfCameras == 1) {
            findViewById(R.id.camera_switch).setVisibility(View.GONE);
            findViewById(R.id.camera_split_view).setVisibility(View.GONE);
        }

        updateStorageSpace(null);

        Open_Camera();
    }

    private void closeCamera() {

        if (null != mCamera) {
            mCamera.closeCamera();
            mCamera = null;
        }
    }


    public void GetCameraCnt() {
        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);

        try {
            CameraIds = manager.getCameraIdList();
            numOfCameras = manager.getCameraIdList().length;
            if (numOfCameras == 0) {
                Toast.makeText(SingleCameraActivity.this, "No camera found, closing the application",
                    Toast.LENGTH_LONG).show();
            }
            Log.d(TAG, "Get total number of cameras present: " + manager.getCameraIdList().length);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void Open_Camera() {
        mCamera_BackView = findViewById(R.id.textureview);
        if (mCamera_BackView == null) {
            Log.e(TAG, "fail to find surface for back camera");
            return;
        }

        if (mCamera == null) {
            Open_Camera_ById();
        }
        if (mCamera_BackView.isAvailable()) {
            mCamera.textureListener.onSurfaceTextureAvailable(
                    mCamera_BackView.getSurfaceTexture(), mCamera_BackView.getWidth(),
                    mCamera_BackView.getHeight());
        } else {
            mCamera_BackView.setSurfaceTextureListener(mCamera.textureListener);
        }
    }

    public void Open_Camera_ById() {
        String[] Data = new String[5];

        Data[0] = "BackCamera";
        Data[1] = CameraIds[mCameraInst.getOpenCameraId()];
        Data[2] = "capture_list";
        Data[3] = "video_list";
        Data[4] = "pref_resolution_1";

        mCamera = new CameraBase(this, mCamera_BackView, null, mRecordingTimeView,
                Data, mRoundedThumbnailView);
    }

    /**
     * Checks if any of the needed Android runtime permissions are missing.
     * If they are, then launch the permissions activity under one of the following conditions:
     * a) The permissions dialogs have not run yet. We will ask for permission only once.
     * b) If the missing permissions are critical to the app running, we will display a fatal error
     * dialog. Critical permissions are: camera, microphone and storage. The app cannot run without
     * them. Non-critical permission is location.
     */
    private void checkPermissions() {
        mHasCriticalPermissions =
                ActivityCompat.checkSelfPermission(getApplicationContext(),
                        Manifest.permission.CAMERA) ==
                        PackageManager.PERMISSION_GRANTED &&
                        ActivityCompat.checkSelfPermission(getApplicationContext(),
                                Manifest.permission.RECORD_AUDIO) ==
                                PackageManager.PERMISSION_GRANTED &&
                        ActivityCompat.checkSelfPermission(getApplicationContext(),
                                Manifest.permission.READ_EXTERNAL_STORAGE) ==
                                PackageManager.PERMISSION_GRANTED;

        if (!mHasCriticalPermissions) {
            Intent intent = new Intent(this, PermissionsActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(intent);
            finish();
        }

        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onCreate: Missing critical permissions.");
            finish();
            return;
        }

    }


    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, " onResume");
    }
}
