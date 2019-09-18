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

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    /**
     * An {@link AutoFitTextureView} for camera preview.
     */
    private AutoFitTextureView mTopLeftCam_textureView, mTopRightCam_textureView,
        mBotmLeftCam_textureView, mBotmRightCam_textureView;

    private Button mTopLeftCam_PictureButton, mTopRightCam_PictureButton,
        mBotmLeftCam_PictureButton, mBotmRightCam_PictureButton, mTopLeftCam_RecordButton,
        mTopRightCam_RecordButton, mBotmLeftCam_RecordButton, mBotmRightCam_RecordButton;

    private int numOfCameras;
    private static final int REQUEST_CAMERA_PERMISSION = 200;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    private FrameLayout frameView0, frameView1, frameView2, frameView3;

    TopLeftCam mTopLeftCam;
    TopRightCam mTopRightCam;
    BotmLeftCam mBotmLeftCam;
    BotmRightCam mBotmRightCam;

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        setVisibilityFrameLayout();
    }

    public void Open_TopLeftCam() {
        mTopLeftCam_textureView = findViewById(R.id.textureview0);
        assert mTopLeftCam_textureView != null;

        mTopLeftCam_PictureButton = findViewById(R.id.Picture0);
        mTopLeftCam_RecordButton = findViewById(R.id.Record0);

        if (isVideoCaptureIntent())
            mTopLeftCam_PictureButton.setVisibility(View.GONE);
        else if (isImageCaptureIntent())
            mTopLeftCam_RecordButton.setVisibility(View.GONE);

        mTopLeftCam = new TopLeftCam(MainActivity.this, mTopLeftCam_textureView,
                                     mTopLeftCam_PictureButton, mTopLeftCam_RecordButton);
    }

    public void Open_TopRightCam() {
        mTopRightCam_textureView = findViewById(R.id.textureview1);
        assert mTopRightCam_textureView != null;

        mTopRightCam_PictureButton = findViewById(R.id.Picture1);
        mTopRightCam_RecordButton = findViewById(R.id.Record1);

        mTopRightCam = new TopRightCam(MainActivity.this, mTopRightCam_textureView,
                                       mTopRightCam_PictureButton, mTopRightCam_RecordButton);
    }

    public void Open_BotmLeftCam() {
        mBotmLeftCam_textureView = findViewById(R.id.textureview2);
        assert mTopRightCam_textureView != null;

        mBotmLeftCam_PictureButton = findViewById(R.id.Picture2);
        mBotmLeftCam_RecordButton = findViewById(R.id.Record2);

        mBotmLeftCam = new BotmLeftCam(MainActivity.this, mBotmLeftCam_textureView,
                                       mBotmLeftCam_PictureButton, mBotmLeftCam_RecordButton);
    }

    public void Open_BotmRightCam() {
        mBotmRightCam_textureView = findViewById(R.id.textureview3);
        assert mTopRightCam_textureView != null;

        mBotmRightCam_PictureButton = findViewById(R.id.Picture3);
        mBotmRightCam_RecordButton = findViewById(R.id.Record3);

        mBotmRightCam = new BotmRightCam(MainActivity.this, mBotmRightCam_textureView,
                                         mBotmRightCam_PictureButton, mBotmRightCam_RecordButton);
    }

    public boolean isVideoCaptureIntent() {
        String action = this.getIntent().getAction();
        return (MediaStore.ACTION_VIDEO_CAPTURE.equals(action));
    }

    public boolean isImageCaptureIntent() {
        String action = this.getIntent().getAction();
        return (MediaStore.ACTION_IMAGE_CAPTURE.equals(action));
    }

    public void setVisibilityFrameLayout() {
        frameView0 = findViewById(R.id.control1);
        frameView1 = findViewById(R.id.control2);
        frameView2 = findViewById(R.id.control3);
        frameView3 = findViewById(R.id.control4);

        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);
        try {
            numOfCameras = manager.getCameraIdList().length;
            // if (numCameras != "") { numOfCameras = Integer.parseInt(numCameras); }
            Log.d(TAG, "onCreate Inside openCamera() Total Cameras: " +
                           manager.getCameraIdList().length);

            if (numOfCameras == 1) {
                frameView1.setVisibility(FrameLayout.INVISIBLE);
                frameView2.setVisibility(FrameLayout.INVISIBLE);
                frameView3.setVisibility(FrameLayout.INVISIBLE);
                Open_TopLeftCam();
            } else if (numOfCameras == 2) {
                frameView2.setVisibility(FrameLayout.INVISIBLE);
                frameView3.setVisibility(FrameLayout.INVISIBLE);
                Open_TopLeftCam();
                Open_TopRightCam();
            } else if (numOfCameras == 3) {
                frameView3.setVisibility(FrameLayout.INVISIBLE);
                Open_TopLeftCam();
                Open_TopRightCam();
                Open_BotmLeftCam();
            } else if (numOfCameras == 4) {
                Open_TopLeftCam();
                Open_TopRightCam();
                Open_BotmLeftCam();
                Open_BotmRightCam();
            } else {
                Log.d(TAG,"No CAMERA CONNECTED");
                frameView0.setVisibility(FrameLayout.INVISIBLE);
                frameView1.setVisibility(FrameLayout.INVISIBLE);
                frameView2.setVisibility(FrameLayout.INVISIBLE);
                frameView3.setVisibility(FrameLayout.INVISIBLE);
            }
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
                                           int[] grantResults) {
        if (requestCode == REQUEST_CAMERA_PERMISSION) {
            if (grantResults[0] == PackageManager.PERMISSION_DENIED) {
                // close the app
                Toast
                    .makeText(MainActivity.this,
                              "Sorry!!!, you can't use this app without granting permission",
                              Toast.LENGTH_LONG)
                    .show();
                finish();
            }
        }
    }

    private void manageTopLeftCam() {
        if (mTopLeftCam == null) {
            Open_TopLeftCam();
            frameView0.setVisibility(FrameLayout.VISIBLE);
        } else if(mTopLeftCam_textureView == null) {
            mTopLeftCam_textureView = findViewById(R.id.textureview0);
            assert mTopLeftCam_textureView != null;
        }

        if (mTopLeftCam_textureView.isAvailable()) {
            frameView0.setVisibility(FrameLayout.VISIBLE);
            mTopLeftCam.openCamera();
        } else {
            mTopLeftCam_textureView.setSurfaceTextureListener(mTopLeftCam.textureListener);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG, "onResume");

        CameraManager manager = (CameraManager)getSystemService(Context.CAMERA_SERVICE);
        try {
            numOfCameras = manager.getCameraIdList().length;
            Log.d(TAG, "Total Cameras: " + manager.getCameraIdList().length);
	} catch (CameraAccessException e) {
            e.printStackTrace();
        }

        if (numOfCameras == 1) {
            manageTopLeftCam();
        } else if (numOfCameras == 2) {
            if (mTopLeftCam_textureView.isAvailable()) {
                mTopLeftCam.openCamera();
            } else {
                mTopLeftCam_textureView.setSurfaceTextureListener(mTopLeftCam.textureListener);
            }

            if (mTopRightCam_textureView.isAvailable()) {
                mTopRightCam.openCamera();
            } else {
                mTopRightCam_textureView.setSurfaceTextureListener(mTopRightCam.textureListener);
            }
        } else {
            Log.d(TAG,"onResume No CAMERA CONNECTED");
            frameView0.setVisibility(FrameLayout.INVISIBLE);
            frameView1.setVisibility(FrameLayout.INVISIBLE);
            frameView2.setVisibility(FrameLayout.INVISIBLE);
            frameView3.setVisibility(FrameLayout.INVISIBLE);
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

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        // noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            Intent intent = new Intent(MainActivity.this, SettingsActivity.class);
            startActivity(intent);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
