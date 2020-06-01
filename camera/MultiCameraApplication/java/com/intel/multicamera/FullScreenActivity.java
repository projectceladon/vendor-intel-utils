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

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
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
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import static com.intel.multicamera.MultiViewActivity.updateStorageSpace;

public class FullScreenActivity extends AppCompatActivity {
    private static final String TAG = "FullScreenActivity";
    private boolean mHasCriticalPermissions;

    public String[] CameraIds;
    private int numOfCameras;
    private AutoFitTextureView mCamera_BackView, mCamera_FrontView;

    private ImageButton mCameraSwitch, mCameraPicture, mCameraRecord, mCameraSplit, mSettings;
    private ImageButton mSettingClose;

    private SettingsPrefUtil Fragment;
    private long mLastClickTime = 0;
    MultiCamera mCameraInst;
    private TextView mRecordingTimeView;
    private boolean mIsRecordingVideo;
    private CameraBase mCameraBack;
    private CameraBase mCameraFront;
    private RoundedThumbnailView mRoundedThumbnailView;
    private BroadcastReceiver mUsbReceiver;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
        }

        setContentView(R.layout.activity_full_screen);

        mIsRecordingVideo = false;
        mCameraInst = MultiCamera.getInstance();
        mCameraSwitch = findViewById(R.id.camera_switch);
        mCameraPicture = findViewById(R.id.Picture);
        mCameraRecord = findViewById(R.id.Record);
        mCameraSplit = findViewById(R.id.camera_split_view);
        mRecordingTimeView = findViewById(R.id.recording_time);
        mSettings = findViewById(R.id.SettingView);
        mSettingClose = findViewById(R.id.SettingClose);
        mRoundedThumbnailView = findViewById(R.id.rounded_thumbnail_view);

        checkPermissions();

        openBackCamera();

        IntentFilter filter = new IntentFilter();
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);

        // BroadcastReceiver when insert/remove the device USB plug into/from a USB port
        mUsbReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                System.out.println("BroadcastReceiver Event");
                if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                    System.out.println(TAG+"BroadcastReceiver USB Connected");
                    openBackCamera();

                } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                    System.out.println(TAG+"BroadcastReceiver USB Disconnected");
                    openBackCamera();
                }
            }
        };

        registerReceiver(mUsbReceiver , filter);

        mCameraSwitch.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                if (SystemClock.elapsedRealtime() - mLastClickTime < 1000) {
                    return;
                }
                mLastClickTime = SystemClock.elapsedRealtime();
                MultiCamera ic_camera = MultiCamera.getInstance();
                if (ic_camera.getWhichCamera() == 0) {
                    openFrontCamera();
                    ic_camera.setWhichCamera(1);
                    Log.i(TAG,"Opened front camera");
                }
                else {
                    openBackCamera();
                    ic_camera.setWhichCamera(0);
                    Log.i(TAG,"Opened back camera");
                }

            }
        });

        mSettings.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                Bundle bundle;
                mSettingClose.setVisibility(View.VISIBLE);

                bundle = new Bundle();
                bundle.putString("Camera_id", CameraIds[0]);
                bundle.putInt("root_preferences", R.xml.root_preferences);
                bundle.putString("pref_resolution", "pref_resolution");
                bundle.putString("video_list", "video_list");
                bundle.putString("capture_list", "capture_list");

                Fragment = new SettingsPrefUtil();
                Fragment.setArguments(bundle);

                getFragmentManager()
                        .beginTransaction()
                        .replace(R.id.PrefScrnSettings, Fragment)
                        .commit();

                mSettings.setVisibility(View.GONE);
                mSettingClose.setVisibility(View.VISIBLE);
                mCameraRecord.setVisibility(View.GONE);
                mCameraPicture.setVisibility(View.GONE);
                mCameraSwitch.setVisibility(View.GONE);
                mCameraSplit.setVisibility(View.GONE);
            }
        });

        mSettingClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                getFragmentManager().beginTransaction().remove(Fragment).commit();

                v.setVisibility(v.GONE);
                mSettings.setVisibility(View.VISIBLE);

                mCameraRecord.setVisibility(View.VISIBLE);
                mCameraPicture.setVisibility(View.VISIBLE);
                mCameraSwitch.setVisibility(View.VISIBLE);
                mCameraSplit.setVisibility(View.VISIBLE);
            }
        });

        mCameraSplit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MultiCamera ic_camera = MultiCamera.getInstance();
                ic_camera.setIsCameraOrSurveillance(1);
                Intent intent = new Intent(FullScreenActivity.this, MultiViewActivity.class);
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
                if (MultiCamera.getInstance().getWhichCamera() == 1) {
                    if (mCameraFront != null)
                        mCameraFront.takePicture();
                }
                else {
                    if (mCameraBack != null)
                        mCameraBack.takePicture();
                }
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
                        if (mCameraInst.getWhichCamera() == 0) {
                            mCameraBack.getmRecord().stopRecordingVideo();
                            mCameraBack.getmRecord().showRecordingUI(false);
                            mCameraBack.getmPhoto().showVideoThumbnail();
                        }
                        else {
                            mCameraFront.getmRecord().stopRecordingVideo();
                            mCameraFront.getmRecord().showRecordingUI(false);
                            mCameraFront.getmPhoto().showVideoThumbnail();
                        }

                        mCameraSwitch.setVisibility(View.VISIBLE);
                        mCameraPicture.setVisibility(View.VISIBLE);
                        mCameraRecord.setImageResource(R.drawable.ic_capture_video);
                        mCameraSplit.setVisibility(View.VISIBLE);
                        mSettings.setVisibility(View.VISIBLE);
                    } else {
                        mIsRecordingVideo =true;
                        if (mCameraInst.getWhichCamera() == 0) {
                            mCameraBack.getmRecord().startRecordingVideo();
                            mCameraBack.getmRecord().showRecordingUI(true);
                        }
                        else {
                            mCameraFront.getmRecord().startRecordingVideo();
                            mCameraFront.getmRecord().showRecordingUI(true);
                        }
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

    public void openBackCamera() {
        closeCamera();

        GetCameraCnt();
        if (numOfCameras == 0) {
            Toast.makeText(FullScreenActivity.this, "No Camera Found", Toast.LENGTH_LONG).show();
            System.out.println(TAG+" no camera found");
            return;
        }
        if (numOfCameras == 1) {
            findViewById(R.id.camera_switch).setVisibility(View.GONE);
            findViewById(R.id.camera_split_view).setVisibility(View.GONE);
        }

        updateStorageSpace(null);

        OpenOnlyBackCamera();
    }

    public void openFrontCamera() {
        closeCamera();

        GetCameraCnt();
        updateStorageSpace(null);

        OpenOnlyFrontCamera();
    }

    private void OpenOnlyFrontCamera() {

        mCamera_FrontView = findViewById(R.id.textureview);
        if (mCamera_FrontView == null) {
            Log.e(TAG, "fail to get surface for front view");
            return;
        }
        if (mCameraFront == null) {
            Open_FrontCamera();
        }

        if (mCamera_FrontView.isAvailable()) {
            mCameraFront.textureListener.onSurfaceTextureAvailable(
                    mCamera_FrontView.getSurfaceTexture(),
                    mCamera_FrontView.getWidth(), mCamera_FrontView.getHeight());
        } else {
            mCamera_FrontView.setSurfaceTextureListener(mCameraFront.textureListener);
        }
    }
    private void closeCamera() {

        if (null != mCameraBack) {
            mCameraBack.closeCamera();
            mCameraBack = null;
        }

        if (null != mCameraFront) {
            mCameraFront.closeCamera();
            mCameraFront = null;
        }
    }


    public void GetCameraCnt() {
        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);

        try {
            CameraIds = manager.getCameraIdList();
            numOfCameras = manager.getCameraIdList().length;
            if (numOfCameras == 0) {
                Toast.makeText(FullScreenActivity.this, "No camera found closing the application",
                    Toast.LENGTH_LONG).show();
            }
            Log.d(TAG, "Get total number of cameras present: " + manager.getCameraIdList().length);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void OpenOnlyBackCamera() {
        mCamera_BackView = findViewById(R.id.textureview);
        if (mCamera_BackView == null) {
            Log.e(TAG, "fail to find surface for back camera");
            return;
        }
        if (mCameraBack == null) {
            Open_BackCamera();
        }
        if (mCamera_BackView.isAvailable()) {
            mCameraBack.textureListener.onSurfaceTextureAvailable(
                    mCamera_BackView.getSurfaceTexture(), mCamera_BackView.getWidth(),
                    mCamera_BackView.getHeight());
        } else {
            mCamera_BackView.setSurfaceTextureListener(mCameraBack.textureListener);
        }
    }

    public void Open_BackCamera() {
        String[] Data = new String[5];

        Data[0] = "BackCamera";
        Data[1] = CameraIds[0];
        Data[2] = "capture_list";
        Data[3] = "video_list";
        Data[4] = "pref_resolution_1";

        mCameraBack = new CameraBase(this, mCamera_BackView, null, mRecordingTimeView,
                Data, mRoundedThumbnailView);
    }


    public void Open_FrontCamera() {
        String[] Data = new String[5];

        Data[0] = "FrontCamera";
        Data[1] = CameraIds[1];
        Data[2] = "capture_list_1";
        Data[3] = "video_list_1";
        Data[4] = "pref_resolution_1";

        mCameraFront = new CameraBase(this, mCamera_FrontView, null, mRecordingTimeView,
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
        unregisterReceiver(mUsbReceiver);
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume");
        MultiCamera ic_cam = MultiCamera.getInstance();
        ic_cam.setIsCameraOrSurveillance(0);
        if (ic_cam.getWhichCamera() == 0) {
            openBackCamera();
        } else {
            openFrontCamera();
        }
    }
}
