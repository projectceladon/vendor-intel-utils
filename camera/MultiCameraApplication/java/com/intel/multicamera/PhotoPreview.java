/*
 * Copyright (C) 2017 The Android Open Source Project
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
import android.graphics.Bitmap;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.net.Uri;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.Surface;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.File;
import java.util.Optional;

public class PhotoPreview {
    private static final String TAG = "PhotoPreview";
    private Uri mCurrentUri;

    private MultiCamera ic_camera;
    private RoundedThumbnailView mRoundedThumbnailView;
    private Activity mActivity;
    FrameLayout roundedThumbnailViewControlLayout;
    private String mCameraId;
    private int mSensorOrientation;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    public PhotoPreview(Activity activity, RoundedThumbnailView roundedThumbnailView, String cameraId) {
        mActivity = activity;
        ic_camera = MultiCamera.getInstance();
        mRoundedThumbnailView = roundedThumbnailView;
        roundedThumbnailViewControlLayout = mActivity.findViewById(R.id.control1);
        mCameraId = cameraId;
        RoundedThumbnail_setOnClickListners();
    }
        private void RoundedThumbnail_setOnClickListners() {
        mRoundedThumbnailView.setCallback(new RoundedThumbnailView.Callback() {
            @Override
            public void onHitStateFinished() {
                System.out.println("RoundedThumbnail_setOnClickListners");
                ImageView preView;
                final ImageButton btnDelete, playButton, btnBack, details;
                FrameLayout previewLayout;

                String mimeType = Utils.getMimeTypeFromURI(mActivity, ic_camera.getCurrentUri());

                previewLayout = mActivity.findViewById(R.id.previewLayout);
                previewLayout.setVisibility(View.VISIBLE);

                btnDelete = mActivity.findViewById(R.id.control_delete);
                playButton = mActivity.findViewById(R.id.play_button);
                preView = mActivity.findViewById(R.id.preview);
                btnBack = mActivity.findViewById(R.id.control_back);
                details = mActivity.findViewById(R.id.control_info);
                details.setVisibility(View.VISIBLE);

                btnBack.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        FrameLayout previewLayout;
                        previewLayout = mActivity.findViewById(R.id.previewLayout);
                        previewLayout.setVisibility(View.GONE);

                        mRoundedThumbnailView.hideThumbnail();
                    }
                });

                btnDelete.setOnClickListener(new View.OnClickListener() {
                    ImageView preView;

                    @Override
                    public void onClick(View v) {
                        preView = mActivity.findViewById(R.id.preview);

                        Uri uri = ic_camera.getCurrentUri();
                        File file = new File(Utils.getRealPathFromURI(mActivity, uri));
                        if (file.exists()) {
                            Log.v(TAG, " Thumbnail Preview File Deleted ");
                            file.delete();
                            // request scan
                            Intent scanIntent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
                            scanIntent.setData(Uri.fromFile(file));
                            mActivity.sendBroadcast(scanIntent);
                            FrameLayout previewLayout;
                            previewLayout = mActivity.findViewById(R.id.previewLayout);
                            previewLayout.setVisibility(View.GONE);

                            mRoundedThumbnailView.hideThumbnail();
                        }
                    }
                });

                details.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        showDetailsDialog(ic_camera.getCurrentFileInfo());
                    }
                });

                if (mimeType.compareTo("video/mp4") == 0) {
                    VideoPreview(playButton, preView);

                } else {
                    photoPreview(playButton, preView);
                }
            }
        });
    }


    private void showDetailsDialog(ContentValues info) {
        Optional<MediaDetails> details = Utils.getMediaDetails(mActivity, info);
        if (!details.isPresent()) {
            return;
        }
        Dialog detailDialog = DetailsDialog.create(mActivity, details.get());
        detailDialog.show();
    }

    private void VideoPreview(ImageButton playButton, ImageView preView) {
        final Optional<Bitmap> bitmap =
                Utils.getVideoThumbnail(mActivity.getContentResolver(), ic_camera.getCurrentUri());
        if (bitmap.isPresent()) {
            playButton.setVisibility(View.VISIBLE);

            playButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Utils.playVideo(mActivity, ic_camera.getCurrentUri(), TAG);
                }
            });

            preView.setVisibility(View.VISIBLE);
            preView.setImageBitmap(bitmap.get());

        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }

    private void photoPreview(ImageButton playButton, ImageView preView) {
        Uri PhotoUri = ic_camera.getCurrentUri();

        preView.setVisibility(View.VISIBLE);
        playButton.setVisibility(View.GONE);
        preView.setImageURI(PhotoUri);
    }

    public void showImageThumbnail(File ImageFile) {
        final int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        final Optional<Bitmap> bitmap =
                Utils.generateThumbnail(ImageFile, roundedThumbnailViewControlLayout.getWidth(),
                        roundedThumbnailViewControlLayout.getMeasuredHeight());

        if (bitmap.isPresent()) {
            mActivity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    mRoundedThumbnailView.startRevealThumbnailAnimation("photo taken");
                    mRoundedThumbnailView.setThumbnail(bitmap.get(), getOrientation(rotation));
                }
            });

        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }

    private int getOrientation(int rotation) {
        // Sensor orientation is 90 for most devices, or 270 for some devices (eg. Nexus 5X)
        // We have to take that into account and rotate JPEG properly.
        // For devices with orientation of 90, we simply return our mapping from ORIENTATIONS.
        // For devices with orientation of 270, we need to rotate the JPEG 180 degrees.
        try {
            CameraManager manager = (CameraManager) mActivity.getSystemService(Context.CAMERA_SERVICE);
            CameraCharacteristics characteristics = manager.getCameraCharacteristics(mCameraId);
            mSensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
        } catch (Exception e) {

        }
        return (ORIENTATIONS.get(rotation) + mSensorOrientation + 270) % 360;
    }


    public void showVideoThumbnail() {
        mRoundedThumbnailView.startRevealThumbnailAnimation("Video taken");

        final Optional<Bitmap> bitmap =
                Utils.getVideoThumbnail(mActivity.getContentResolver(), ic_camera.getCurrentUri());

        if (bitmap.isPresent()) {
            mRoundedThumbnailView.setThumbnail(bitmap.get(), 0);
        } else {
            Log.e(TAG, "No bitmap image found: ");
        }
    }
}
