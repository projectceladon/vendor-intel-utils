/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.camera.one.v2.photo;

import android.hardware.camera2.CameraDevice;

import com.android.camera.async.MainThread;
import com.android.camera.debug.Logger;
import com.android.camera.one.OneCamera;
import com.android.camera.one.v2.commands.CameraCommandExecutor;
import com.android.camera.one.v2.core.FrameServer;
import com.android.camera.one.v2.core.RequestBuilder;
import com.android.camera.one.v2.imagesaver.ImageSaver;
import com.android.camera.one.v2.sharedimagereader.ManagedImageReader;
import com.google.common.base.Supplier;

import java.util.Arrays;

public final class PictureTakerFactory {
    private final PictureTakerImpl mPictureTaker;

    private PictureTakerFactory(PictureTakerImpl pictureTaker) {
        mPictureTaker = pictureTaker;
    }

    public static PictureTakerFactory create(Logger.Factory logFactory, MainThread mainExecutor,
            CameraCommandExecutor commandExecutor,
            ImageSaver.Builder imageSaverBuilder,
            FrameServer frameServer,
            RequestBuilder.Factory rootRequestBuilder,
            ManagedImageReader sharedImageReader,
            Supplier<OneCamera.PhotoCaptureParameters.Flash> flashMode, boolean cafSupport) {
        // When flash is ON, always use the ConvergedImageCaptureCommand which
        // performs the AE precapture sequence and AF precapture if supported.
        ImageCaptureCommand flashOnCommand = cafSupport ? new ConvergedImageCaptureCommand(
                sharedImageReader, frameServer, rootRequestBuilder,
                CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                Arrays.asList(rootRequestBuilder), true /* ae */, true /* af */) :
            new ConvergedImageCaptureCommand(
                    sharedImageReader, frameServer, rootRequestBuilder,
                    CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                    CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                    Arrays.asList(rootRequestBuilder), true /* ae */);

        // When flash is OFF, wait for AF convergence if AF is supported, but not AE convergence
        // (which can be very slow).
        ImageCaptureCommand flashOffCommand = cafSupport ? new ConvergedImageCaptureCommand(
                sharedImageReader, frameServer, rootRequestBuilder,
                CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                Arrays.asList(rootRequestBuilder), false /* ae */, true /* af */) :
            new ConvergedImageCaptureCommand(
                    sharedImageReader, frameServer, rootRequestBuilder,
                    CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                    CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                    Arrays.asList(rootRequestBuilder), false /* ae */);

        // When flash is AUTO, wait for AE and AF if supported.
        // TODO OPTIMIZE If the last converged-AE state indicates that flash is
        // not necessary, then this could skip waiting for AE convergence.
        ImageCaptureCommand flashAutoCommand = cafSupport ? new ConvergedImageCaptureCommand(
                sharedImageReader, frameServer, rootRequestBuilder,
                CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                Arrays.asList(rootRequestBuilder), true /* ae */, true /* af */) :
            new ConvergedImageCaptureCommand(
                    sharedImageReader, frameServer, rootRequestBuilder,
                    CameraDevice.TEMPLATE_PREVIEW /* repeatingRequestTemplate */,
                    CameraDevice.TEMPLATE_STILL_CAPTURE /* stillCaptureRequestTemplate */,
                    Arrays.asList(rootRequestBuilder), true /* ae */);

        ImageCaptureCommand flashBasedCommand = new FlashBasedPhotoCommand(logFactory, flashMode,
                flashOnCommand, flashAutoCommand, flashOffCommand);
        return new PictureTakerFactory(new PictureTakerImpl(mainExecutor, commandExecutor,
                imageSaverBuilder, flashBasedCommand));
    }

    public PictureTaker providePictureTaker() {
        return mPictureTaker;
    }
}
