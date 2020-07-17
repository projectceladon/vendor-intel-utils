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

import android.content.ContentValues;
import android.graphics.Camera;
import android.net.Uri;

public class MultiCamera {
    private static MultiCamera ic_instance = null;
    private static final int usb_dev_id = 239;
    private CameraBase mTopRightCam;
    private CameraBase mBotmLeftCam;
    private CameraBase mBotmRightCam;
    private CameraBase mTopLeftCam;

    private int mOpenCameraId;
    private Uri mCurrentUri;
    private ContentValues mCurrentFileInfo;

    MultiCamera() {
        mWhichCamera = 0;
        mIsCameraOrSurveillance = 0;
        mOpenCameraId = -1;
    }
    public static MultiCamera getInstance() {
        if (ic_instance == null) {
            ic_instance = new MultiCamera();
        }

        return ic_instance;
    }

    private int mWhichCamera;

    private int mIsCameraOrSurveillance;

    public int getWhichCamera() {
        return mWhichCamera;
    }

    public void setWhichCamera(int whichCamera) {
        mWhichCamera = whichCamera;
    }

    public int getIsCameraOrSurveillance() {
        return mIsCameraOrSurveillance;
    }

    public void setIsCameraOrSurveillance(int isCameraOrSurveillance) {
        mIsCameraOrSurveillance = isCameraOrSurveillance;
    }

    public Uri getCurrentUri() {
        return mCurrentUri;
    }

    public void setCurrentUri(Uri currentUri) {
        this.mCurrentUri = currentUri;
    }

    public ContentValues getCurrentFileInfo() {
        return mCurrentFileInfo;
    }

    public void setCurrentFileInfo(ContentValues currentFileInfo) {
        this.mCurrentFileInfo = currentFileInfo;
    }

    public CameraBase getTopRightCam() {
        return mTopRightCam;
    }

    public void setTopRightCam(CameraBase topRightCam) {
        this.mTopRightCam = topRightCam;
    }

    public CameraBase getBotLeftCam() {
        return mBotmLeftCam;
    }

    public void setBotLeftCam(CameraBase botmLeftCam) {
        this.mBotmLeftCam = botmLeftCam;
    }

    public CameraBase getBotRightCam() {
        return mBotmRightCam;
    }

    public void setBotRightCam(CameraBase botmRightCam) {
        this.mBotmRightCam = botmRightCam;
    }

    public CameraBase getTopLeftCam() {
        return mTopLeftCam;
    }

    public void setTopLeftCam(CameraBase mTopLeftCam) {
        this.mTopLeftCam = mTopLeftCam;
    }

    public int getOpenCameraId() {
        return mOpenCameraId;
    }

    public void setOpenCameraId(int openCameraId) {
        mOpenCameraId = openCameraId;
    }
    public static int getUsbCamDeviceClass() {
        return usb_dev_id;
    }
}
