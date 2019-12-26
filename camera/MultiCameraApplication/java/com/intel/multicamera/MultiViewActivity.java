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
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.*;
import android.widget.*;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import java.io.IOException;

public class MultiViewActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView mTopLeftCam_textureView, mTopRightCam_textureView,
            mBotmLeftCam_textureView, mBotmRightCam_textureView;

    private ImageButton mTopLeftCam_RecordButton, mTopLeftCam_PictureButton,
            mTopRightCam_PictureButton, mBotmLeftCam_PictureButton, mBotmRightCam_PictureButton,
            mTopRightCam_RecordButton, mBotmLeftCam_RecordButton, mBotmRightCam_RecordButton;

    private ImageButton SettingView0, SettingView1, SettingView2, SettingView3, SettingClose0,
            SettingClose1, SettingClose2, SettingClose3, FullScrn0, FullScrn1, FullScrn2, FullScrn3,
            exitScrn0, exitScrn1, exitScrn2, exitScrn3;

    private TextView mRecordingTimeView, mRecordingTimeView0, mRecordingTimeView1,
            mRecordingTimeView2;

    private int numOfCameras;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    private FrameLayout frameView0, frameView1, frameView2, frameView3;

    private CameraBase mTopRightCam, mBotmLeftCam, mBotmRightCam, mTopLeftCam;

    private SettingsPrefUtil Fragment, Fragment1, Fragment2, Fragment3;

    public String[] CameraIds;
    private boolean mHasCriticalPermissions;

    private static final Object mStorageSpaceLock = new Object();
    private static long mStorageSpaceBytes = Utils.LOW_STORAGE_THRESHOLD_BYTES;

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    private int[] FrameVisibility;
    private boolean exitScrnFlag;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
        }

        setContentView(R.layout.activity_multiview);

        Log.e(TAG, "onCreate");

        checkPermissions();
        if (!mHasCriticalPermissions) {
            Log.v(TAG, "onCreate: Missing critical permissions.");
            finish();
            return;
        }

        Settings_Init();

        FullScrn_Init();

        set_FrameVisibilities();
    }

    private void set_FrameVisibilities() {
        FrameVisibility = new int[4];

        frameView0 = findViewById(R.id.control1);
        frameView1 = findViewById(R.id.control2);
        frameView2 = findViewById(R.id.control3);
        frameView3 = findViewById(R.id.control4);

        FrameVisibility[0] = frameView0.getVisibility();
        FrameVisibility[1] = frameView1.getVisibility();
        FrameVisibility[2] = frameView2.getVisibility();
        FrameVisibility[3] = frameView3.getVisibility();
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
            startActivity(intent);
            finish();
        }
    }

    private void FullScrn_Init() {
        FullScrn0 = findViewById(R.id.imageView0);
        FullScrn1 = findViewById(R.id.imageView1);
        FullScrn2 = findViewById(R.id.imageView2);
        FullScrn3 = findViewById(R.id.imageView3);

        exitScrn0 = findViewById(R.id.exitFullScreen0);
        exitScrn1 = findViewById(R.id.exitFullScreen1);
        exitScrn2 = findViewById(R.id.exitFullScreen2);
        exitScrn3 = findViewById(R.id.exitFullScreen3);
    }

    private void Settings_Init() {
        SettingView0 = findViewById(R.id.SettingView0);
        SettingView1 = findViewById(R.id.SettingView1);
        SettingView2 = findViewById(R.id.SettingView2);
        SettingView3 = findViewById(R.id.SettingView3);

        SettingClose0 = findViewById(R.id.mSettingClose0);
        SettingClose1 = findViewById(R.id.mSettingClose1);
        SettingClose2 = findViewById(R.id.mSettingClose2);
        SettingClose3 = findViewById(R.id.mSettingClose3);
    }

    public void GetCameraCnt() {
        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);

        try {
            CameraIds = manager.getCameraIdList();
            numOfCameras = manager.getCameraIdList().length;
            Log.d(TAG, "Inside Settings Total Cameras: " + manager.getCameraIdList().length);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void Open_TopLeftCam() {
        String[] Data = new String[5];
        ImageButton[] Buttons = new ImageButton[4];

        mTopLeftCam_textureView = findViewById(R.id.textureview0);
        if (mTopLeftCam_textureView == null) return;

        mTopLeftCam_PictureButton = findViewById(R.id.Picture0);
        mTopLeftCam_RecordButton = findViewById(R.id.Record0);

        Buttons[0] = mTopLeftCam_PictureButton;
        Buttons[1] = mTopLeftCam_RecordButton;
        Buttons[2] = SettingView0;
        Buttons[3] = FullScrn0;

        mRecordingTimeView = findViewById(R.id.recording_time);

        Data[0] = "TopLeftCam";
        Data[1] = CameraIds[0];
        Data[2] = "capture_list";
        Data[3] = "video_list";
        Data[4] = "pref_resolution";

        RoundedThumbnailView roundedThumbnailView = findViewById(R.id.rounded_thumbnail_view);

        mTopLeftCam = new CameraBase(this, mTopLeftCam_textureView, Buttons, mRecordingTimeView,
                                     Data, roundedThumbnailView);
    }

    public void Open_TopRightCam() {
        String[] Data = new String[5];
        ImageButton[] Buttons = new ImageButton[4];
        mTopRightCam_textureView = findViewById(R.id.textureview1);
        if (mTopRightCam_textureView == null) return;

        mTopRightCam_PictureButton = findViewById(R.id.Picture1);
        mTopRightCam_RecordButton = findViewById(R.id.Record1);

        Buttons[0] = mTopRightCam_PictureButton;
        Buttons[1] = mTopRightCam_RecordButton;
        Buttons[2] = SettingView1;
        Buttons[3] = FullScrn1;

        Data[0] = "TopRightCam";
        Data[1] = CameraIds[1];
        Data[2] = "capture_list_1";
        Data[3] = "video_list_1";
        Data[4] = "pref_resolution_1";

        mRecordingTimeView0 = findViewById(R.id.recording_time0);

        RoundedThumbnailView roundedThumbnailView = findViewById(R.id.rounded_thumbnail_view0);

        mTopRightCam = new CameraBase(this, mTopRightCam_textureView, Buttons, mRecordingTimeView0,
                                      Data, roundedThumbnailView);
    }

    public void Open_BotmLeftCam() {
        String[] Data = new String[5];
        ImageButton[] Buttons = new ImageButton[4];
        mBotmLeftCam_textureView = findViewById(R.id.textureview2);
        if (mBotmLeftCam_textureView == null) return;

        mBotmLeftCam_PictureButton = findViewById(R.id.Picture2);
        mBotmLeftCam_RecordButton = findViewById(R.id.Record2);

        Buttons[0] = mBotmLeftCam_PictureButton;
        Buttons[1] = mBotmLeftCam_RecordButton;
        Buttons[2] = SettingView2;
        Buttons[3] = FullScrn2;

        Data[0] = "BotmLeftCam";
        Data[1] = CameraIds[2];
        Data[2] = "capture_list_2";
        Data[3] = "video_list_2";
        Data[4] = "pref_resolution_2";

        mRecordingTimeView1 = findViewById(R.id.recording_time1);

        RoundedThumbnailView roundedThumbnailView = findViewById(R.id.rounded_thumbnail_view1);

        mBotmLeftCam = new CameraBase(this, mBotmLeftCam_textureView, Buttons, mRecordingTimeView1,
                                      Data, roundedThumbnailView);
    }

    public void Open_BotmRightCam() {
        String[] Data = new String[5];
        ImageButton[] Buttons = new ImageButton[4];
        mBotmRightCam_textureView = findViewById(R.id.textureview3);
        if (mTopRightCam_textureView == null) return;

        mBotmRightCam_PictureButton = findViewById(R.id.Picture3);
        mBotmRightCam_RecordButton = findViewById(R.id.Record3);

        Buttons[0] = mBotmRightCam_PictureButton;
        Buttons[1] = mBotmRightCam_RecordButton;
        Buttons[2] = SettingView3;
        Buttons[3] = FullScrn3;

        Data[0] = "BotmRightCam";
        Data[1] = CameraIds[3];
        Data[2] = "capture_list_3";
        Data[3] = "video_list_3";
        Data[4] = "pref_resolution_3";

        mRecordingTimeView2 = findViewById(R.id.recording_time2);

        RoundedThumbnailView roundedThumbnailView = findViewById(R.id.rounded_thumbnail_view2);

        mBotmRightCam = new CameraBase(this, mBotmRightCam_textureView, Buttons,
                                       mRecordingTimeView2, Data, roundedThumbnailView);
    }

    private void manageTopLeftCam() {
        frameView0.setVisibility(FrameLayout.VISIBLE);
        FrameVisibility[0] = frameView0.getVisibility();
        if (mTopLeftCam == null) {
            Open_TopLeftCam();
        } else if (mTopLeftCam_textureView == null) {
            mTopLeftCam_textureView = findViewById(R.id.textureview0);
        }

        if (mTopLeftCam_textureView.isAvailable()) {
            mTopLeftCam.textureListener.onSurfaceTextureAvailable(
                    mTopLeftCam_textureView.getSurfaceTexture(), mTopLeftCam_textureView.getWidth(),
                    mTopLeftCam_textureView.getHeight());
        } else {
            mTopLeftCam_textureView.setSurfaceTextureListener(mTopLeftCam.textureListener);
        }
    }

    private void manageTopRightCam() {
        frameView1.setVisibility(FrameLayout.VISIBLE);
        FrameVisibility[1] = frameView1.getVisibility();
        if (mTopRightCam == null) {
            Open_TopRightCam();

        } else if (mTopRightCam_textureView == null) {
            mTopRightCam_textureView = findViewById(R.id.textureview1);
        }

        if (mTopRightCam_textureView.isAvailable()) {
            mTopRightCam.textureListener.onSurfaceTextureAvailable(
                    mTopRightCam_textureView.getSurfaceTexture(),
                    mTopRightCam_textureView.getWidth(), mTopRightCam_textureView.getHeight());
        } else {
            mTopRightCam_textureView.setSurfaceTextureListener(mTopRightCam.textureListener);
        }
    }

    private void manageBotmLeftCam() {
        frameView2.setVisibility(FrameLayout.VISIBLE);
        FrameVisibility[2] = frameView2.getVisibility();
        if (mBotmLeftCam == null) {
            Open_BotmLeftCam();

        } else if (mBotmLeftCam_textureView == null) {
            mBotmLeftCam_textureView = findViewById(R.id.textureview2);
        }

        if (mBotmLeftCam_textureView.isAvailable()) {
            mBotmLeftCam.textureListener.onSurfaceTextureAvailable(
                    mBotmLeftCam_textureView.getSurfaceTexture(),
                    mBotmLeftCam_textureView.getWidth(), mBotmLeftCam_textureView.getHeight());
        } else {
            mBotmLeftCam_textureView.setSurfaceTextureListener(mBotmLeftCam.textureListener);
        }
    }

    private void manageBotmRightCam() {
        frameView3.setVisibility(FrameLayout.VISIBLE);
        FrameVisibility[3] = frameView3.getVisibility();
        if (mBotmRightCam == null) {
            Open_BotmRightCam();

        } else if (mBotmRightCam_textureView == null) {
            mBotmRightCam_textureView = findViewById(R.id.textureview3);
        }

        if (mBotmRightCam_textureView.isAvailable()) {
            mBotmRightCam.textureListener.onSurfaceTextureAvailable(
                    mBotmRightCam_textureView.getSurfaceTexture(),
                    mBotmRightCam_textureView.getWidth(), mBotmRightCam_textureView.getHeight());
        } else {
            mBotmRightCam_textureView.setSurfaceTextureListener(mBotmRightCam.textureListener);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume");

        GetCameraCnt();
        updateStorageSpace(null);

        if (numOfCameras == 1) {
            manageTopLeftCam();
        } else if (numOfCameras == 2) {
            manageTopLeftCam();
            manageTopRightCam();
        } else if (numOfCameras == 3) {
            manageTopLeftCam();
            manageBotmLeftCam();
            manageTopRightCam();
        } else if (numOfCameras == 4) {
            manageTopLeftCam();
            manageTopRightCam();
            manageBotmLeftCam();
            manageBotmRightCam();

        } else {
            Log.d(TAG, "onResume No CAMERA CONNECTED");
        }
    }

    private void closeCamera() {
        if (null != mTopLeftCam) mTopLeftCam.closeCamera();

        if (null != mTopRightCam) mTopRightCam.closeCamera();

        if (null != mBotmRightCam) mBotmRightCam.closeCamera();

        if (null != mBotmLeftCam) mBotmLeftCam.closeCamera();
    }

    @Override
    protected void onPause() {
        Log.e(TAG, "onPause");
        super.onPause();

        closeCamera();
    }

    public void settingView(View view) {
        FrameLayout frameLayout;
        Bundle bundle;

        switch (view.getId()) {
            case R.id.SettingView0:
                frameLayout = findViewById(R.id.PrefScrnSettings0);

                frameLayout.setVisibility(View.VISIBLE);
                SettingClose0.setVisibility(View.VISIBLE);

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
                        .replace(R.id.PrefScrnSettings0, Fragment)
                        .commit();

                SettingView0.setVisibility(View.GONE);
                FullScrn0.setVisibility(View.GONE);
                if (exitScrnFlag) exitScrn0.setVisibility(View.INVISIBLE);
                mTopLeftCam_RecordButton.setVisibility(View.GONE);
                mTopLeftCam_PictureButton.setVisibility(View.GONE);
                break;
            case R.id.SettingView1:
                frameLayout = findViewById(R.id.PrefScrnSettings1);

                frameLayout.setVisibility(View.VISIBLE);
                SettingClose1.setVisibility(View.VISIBLE);

                bundle = new Bundle();
                bundle.putString("Camera_id", CameraIds[1]);
                bundle.putInt("root_preferences", R.xml.root_preferences_1);
                bundle.putString("pref_resolution", "pref_resolution_1");
                bundle.putString("video_list", "video_list_1");
                bundle.putString("capture_list", "capture_list_1");

                Fragment1 = new SettingsPrefUtil();
                Fragment1.setArguments(bundle);

                getFragmentManager()
                        .beginTransaction()
                        .replace(R.id.PrefScrnSettings1, Fragment1)
                        .commit();

                SettingView1.setVisibility(View.GONE);
                FullScrn1.setVisibility(View.GONE);
                if (exitScrnFlag) exitScrn1.setVisibility(View.INVISIBLE);
                mTopRightCam_RecordButton.setVisibility(View.GONE);
                mTopRightCam_PictureButton.setVisibility(View.GONE);
                break;
            case R.id.SettingView2:
                frameLayout = findViewById(R.id.PrefScrnSettings2);

                frameLayout.setVisibility(View.VISIBLE);
                SettingClose2.setVisibility(View.VISIBLE);

                bundle = new Bundle();
                bundle.putString("Camera_id", CameraIds[2]);
                bundle.putInt("root_preferences", R.xml.root_preferences_2);
                bundle.putString("pref_resolution", "pref_resolution_2");
                bundle.putString("video_list", "video_list_2");
                bundle.putString("capture_list", "capture_list_2");

                Fragment2 = new SettingsPrefUtil();
                Fragment2.setArguments(bundle);

                getFragmentManager()
                        .beginTransaction()
                        .replace(R.id.PrefScrnSettings2, Fragment2)
                        .commit();

                SettingView2.setVisibility(View.GONE);
                FullScrn2.setVisibility(View.GONE);
                if (exitScrnFlag) exitScrn2.setVisibility(View.INVISIBLE);
                mBotmLeftCam_RecordButton.setVisibility(View.GONE);
                mBotmLeftCam_PictureButton.setVisibility(View.GONE);
                break;
            case R.id.SettingView3:
                frameLayout = findViewById(R.id.PrefScrnSettings3);

                frameLayout.setVisibility(View.VISIBLE);
                SettingClose3.setVisibility(View.VISIBLE);

                bundle = new Bundle();
                bundle.putString("Camera_id", CameraIds[3]);
                bundle.putInt("root_preferences", R.xml.root_preferences_3);
                bundle.putString("pref_resolution", "pref_resolution_3");
                bundle.putString("video_list", "video_list_3");
                bundle.putString("capture_list", "capture_list_3");

                Fragment3 = new SettingsPrefUtil();
                Fragment3.setArguments(bundle);

                getFragmentManager()
                        .beginTransaction()
                        .replace(R.id.PrefScrnSettings3, Fragment3)
                        .commit();

                SettingView3.setVisibility(View.GONE);
                FullScrn3.setVisibility(View.GONE);
                if (exitScrnFlag) exitScrn3.setVisibility(View.INVISIBLE);
                mBotmRightCam_RecordButton.setVisibility(View.GONE);
                mBotmRightCam_PictureButton.setVisibility(View.GONE);
                break;

            default:
                break;
        }
    }

    public void settingClose(View view) {
        FrameLayout frameLayout;

        switch (view.getId()) {
            case R.id.mSettingClose0:
                frameLayout = findViewById(R.id.PrefScrnSettings0);

                getFragmentManager().beginTransaction().remove(Fragment).commit();

                frameLayout.setVisibility(View.GONE);
                view.setVisibility(view.GONE);
                SettingView0.setVisibility(View.VISIBLE);

                if (!exitScrnFlag)
                    FullScrn0.setVisibility(View.VISIBLE);
                else
                    exitScrn0.setVisibility(View.VISIBLE);

                mTopLeftCam_RecordButton.setVisibility(View.VISIBLE);
                mTopLeftCam_PictureButton.setVisibility(View.VISIBLE);

                mTopLeftCam.createCameraPreview();

                break;
            case R.id.mSettingClose1:
                frameLayout = findViewById(R.id.PrefScrnSettings1);

                getFragmentManager().beginTransaction().remove(Fragment1).commit();

                frameLayout.setVisibility(View.GONE);
                view.setVisibility(view.GONE);
                SettingView1.setVisibility(View.VISIBLE);

                if (!exitScrnFlag)
                    FullScrn1.setVisibility(View.VISIBLE);
                else
                    exitScrn1.setVisibility(View.VISIBLE);

                mTopRightCam_RecordButton.setVisibility(View.VISIBLE);
                mTopRightCam_PictureButton.setVisibility(View.VISIBLE);

                mTopRightCam.createCameraPreview();

                break;
            case R.id.mSettingClose2:
                frameLayout = findViewById(R.id.PrefScrnSettings2);
                getFragmentManager().beginTransaction().remove(Fragment2).commit();

                frameLayout.setVisibility(View.GONE);
                view.setVisibility(View.GONE);
                SettingView2.setVisibility(View.VISIBLE);

                if (!exitScrnFlag)
                    FullScrn2.setVisibility(View.VISIBLE);
                else
                    exitScrn2.setVisibility(View.VISIBLE);

                mBotmLeftCam_RecordButton.setVisibility(View.VISIBLE);
                mBotmLeftCam_PictureButton.setVisibility(View.VISIBLE);

                mBotmLeftCam.createCameraPreview();

                break;
            case R.id.mSettingClose3:
                frameLayout = findViewById(R.id.PrefScrnSettings3);
                getFragmentManager().beginTransaction().remove(Fragment3).commit();

                frameLayout.setVisibility(View.GONE);
                view.setVisibility(view.GONE);
                SettingView3.setVisibility(View.VISIBLE);

                if (!exitScrnFlag)
                    FullScrn3.setVisibility(View.VISIBLE);
                else
                    exitScrn3.setVisibility(View.VISIBLE);

                mBotmRightCam_RecordButton.setVisibility(View.VISIBLE);
                mBotmRightCam_PictureButton.setVisibility(View.VISIBLE);

                mBotmRightCam.createCameraPreview();

                break;

            default:
                break;
        }
    }

    public void enterFullScreen(View view) {
        LinearLayout LinLayout1, LinLayout2;

        LinLayout1 = findViewById(R.id.TopLayout);

        LinLayout2 = findViewById(R.id.BtmLayout);

        switch (view.getId()) {
            case R.id.imageView0:
                this.setTitle("TopLeftCam");
                exitScrnFlag = true;
                exitScrn0.setVisibility(View.VISIBLE);
                FullScrn0.setVisibility(View.GONE);

                frameView1.setVisibility(View.GONE);

                LinLayout2.setVisibility(View.GONE);

                break;
            case R.id.exitFullScreen0:
                this.setTitle("MultiCamera");
                exitScrnFlag = false;

                FullScrn0.setVisibility(View.VISIBLE);

                exitScrn0.setVisibility(View.GONE);

                switch (FrameVisibility[1]) {
                    case View.VISIBLE:

                        frameView1.setVisibility(View.VISIBLE);

                        break;

                    case View.INVISIBLE:

                        frameView1.setVisibility(View.INVISIBLE);

                        break;
                    default:
                        break;
                }

                FrameVisibility[1] = frameView1.getVisibility();

                LinLayout2.setVisibility(View.VISIBLE);

                break;
            case R.id.imageView1:
                this.setTitle("TopRightCam");
                exitScrnFlag = true;
                FullScrn1.setVisibility(View.GONE);
                exitScrn1.setVisibility(View.VISIBLE);

                frameView0.setVisibility(View.GONE);
                LinLayout2.setVisibility(View.GONE);

                break;
            case R.id.exitFullScreen1:
                this.setTitle("MultiCamera");

                exitScrnFlag = false;
                FullScrn1.setVisibility(View.VISIBLE);
                exitScrn1.setVisibility(View.GONE);

                switch (FrameVisibility[0]) {
                    case View.VISIBLE:

                        frameView0.setVisibility(View.VISIBLE);

                        break;

                    case View.INVISIBLE:

                        frameView0.setVisibility(View.INVISIBLE);

                        break;
                    default:
                        break;
                }

                FrameVisibility[0] = frameView0.getVisibility();

                LinLayout2.setVisibility(View.VISIBLE);

                break;

            case R.id.imageView2:
                this.setTitle("BotmLeftCam");

                exitScrnFlag = true;
                FullScrn2.setVisibility(View.GONE);
                exitScrn2.setVisibility(View.VISIBLE);

                frameView3.setVisibility(View.GONE);
                LinLayout1.setVisibility(View.GONE);

                break;
            case R.id.exitFullScreen2:
                this.setTitle("MultiCamera");

                exitScrnFlag = false;
                FullScrn2.setVisibility(View.VISIBLE);
                exitScrn2.setVisibility(View.GONE);

                switch (FrameVisibility[3]) {
                    case View.VISIBLE:

                        frameView3.setVisibility(View.VISIBLE);

                        break;

                    case View.INVISIBLE:

                        frameView3.setVisibility(View.INVISIBLE);

                        break;
                    default:
                        break;
                }

                FrameVisibility[3] = frameView3.getVisibility();

                LinLayout1.setVisibility(View.VISIBLE);

                break;
            case R.id.imageView3:
                this.setTitle("BotmRightCam");
                exitScrnFlag = true;
                FullScrn3.setVisibility(View.GONE);
                exitScrn3.setVisibility(View.VISIBLE);

                frameView2.setVisibility(View.GONE);
                LinLayout1.setVisibility(View.GONE);

                break;
            case R.id.exitFullScreen3:
                this.setTitle("MultiCamera");
                exitScrnFlag = false;
                FullScrn3.setVisibility(View.VISIBLE);
                exitScrn3.setVisibility(View.GONE);

                switch (FrameVisibility[2]) {
                    case View.VISIBLE:

                        frameView2.setVisibility(View.VISIBLE);

                        break;

                    case View.INVISIBLE:

                        frameView2.setVisibility(View.INVISIBLE);

                        break;
                    default:
                        break;
                }

                FrameVisibility[2] = frameView2.getVisibility();

                LinLayout1.setVisibility(View.VISIBLE);

                break;
            default:
                break;
        }
    }

    protected static long getStorageSpaceBytes() {
        synchronized (mStorageSpaceLock) {
            return mStorageSpaceBytes;
        }
    }

    protected interface OnStorageUpdateDoneListener {
        void onStorageUpdateDone(long bytes) throws IOException, CameraAccessException;
    }

    protected static void updateStorageSpace(final OnStorageUpdateDoneListener callback) {
        /*
         * We execute disk operations on a background thread in order to
         * free up the UI thread.  Synchronizing on the lock below ensures
         * that when getStorageSpaceBytes is called, the main thread waits
         * until this method has completed.
         *
         * However, .execute() does not ensure this execution block will be
         * run right away (.execute() schedules this AsyncTask for sometime
         * in the future. executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR)
         * tries to execute the task in parellel with other AsyncTasks, but
         * there's still no guarantee).
         * e.g. don't call this then immediately call getStorageSpaceBytes().
         * Instead, pass in an OnStorageUpdateDoneListener.
         */
        (new AsyncTask<Void, Void, Long>() {
            @Override
            protected Long doInBackground(Void... arg) {
                synchronized (mStorageSpaceLock) {
                    mStorageSpaceBytes = Utils.getAvailableSpace();
                    return mStorageSpaceBytes;
                }
            }

            @Override
            protected void onPostExecute(Long bytes) {
                // This callback returns after I/O to check disk, so we could be
                // pausing and shutting down. If so, don't bother invoking.
                if (callback != null) {
                    try {
                        callback.onStorageUpdateDone(bytes);
                    } catch (IOException e) {
                        e.printStackTrace();
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                } else {
                    Log.v(TAG, "ignoring storage callback after activity pause");
                }
            }
        }).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
}
