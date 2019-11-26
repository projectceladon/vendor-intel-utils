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

import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

public class CtsCameraIntentsActivity extends AppCompatActivity {
    private static final String TAG = "CameraFullSrnActivity";
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView mCam_textureView;

    private ImageView mCam_PictureButton, mCam_RecordButton;

    private CtsCamIntents CamIntents;

    private TextView mRecordingTimeView;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_multiview);
        setContentView(R.layout.activity_itscameraintents);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        View decorView = getWindow().getDecorView();

        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);

        mCam_textureView = findViewById(R.id.textureview0);
        if (mCam_textureView == null) return;

        mCam_PictureButton = findViewById(R.id.Picture0);
        mCam_RecordButton = findViewById(R.id.Record0);

        Open_Cam();
    }

    public boolean isVideoCaptureIntent() {
        String action = this.getIntent().getAction();
        ;
        return (MediaStore.ACTION_VIDEO_CAPTURE.equals(action));
    }

    public boolean isImageCaptureIntent() {
        String action = this.getIntent().getAction();
        return (MediaStore.ACTION_IMAGE_CAPTURE.equals(action));
    }

    public void Open_Cam() {
        this.setTitle("CtsCamIntents");

        if (isVideoCaptureIntent())
            mCam_PictureButton.setVisibility(View.GONE);
        else if (isImageCaptureIntent())
            mCam_RecordButton.setVisibility(View.GONE);

        mRecordingTimeView = findViewById(R.id.recording_time);

        CamIntents = new CtsCamIntents(this, mCam_textureView, mCam_PictureButton,
                                       mCam_RecordButton, mRecordingTimeView);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.e(TAG, "onPause");
        closeCamera();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");

        if (mCam_textureView.isAvailable()) {
            CamIntents.textureListener.onSurfaceTextureAvailable(
                    mCam_textureView.getSurfaceTexture(), mCam_textureView.getWidth(),
                    mCam_textureView.getHeight());
        } else {
            mCam_textureView.setSurfaceTextureListener(CamIntents.textureListener);
        }
    }

    private void closeCamera() {
        if (null != CamIntents) CamIntents.closeCamera();
    }
}
