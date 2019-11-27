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

import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import javax.microedition.khronos.egl.*;

public class SurfaceUtil {
    private static final int[] ATTRIB_LIST = new int[] {
            EGL10.EGL_RED_SIZE,   8, EGL10.EGL_GREEN_SIZE, 8, EGL10.EGL_BLUE_SIZE, 8,
            EGL10.EGL_ALPHA_SIZE, 8, EGL10.EGL_NONE,
    };

    // http://stackoverflow.com/a/21564236/2681195
    public static void clear(SurfaceTexture mTexture) {
        EGL10 egl = (EGL10)EGLContext.getEGL();
        EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        int[] version = new int[2];
        egl.eglInitialize(display, version);

        EGLConfig[] configs = new EGLConfig[1];
        int[] numConfig = new int[1];
        egl.eglChooseConfig(display, ATTRIB_LIST, configs, 1, numConfig);

        EGLSurface surface = egl.eglCreateWindowSurface(display, configs[0], mTexture, null);
        EGLContext context = egl.eglCreateContext(display, configs[0], EGL10.EGL_NO_CONTEXT, null);

        egl.eglMakeCurrent(display, surface, surface, context);

        GLES20.glClearColor(0, 0, 1, 0);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);

        egl.eglSwapBuffers(display, surface);
        egl.eglMakeCurrent(display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE,
                           EGL10.EGL_NO_CONTEXT);
        egl.eglDestroyContext(display, context);
        egl.eglDestroySurface(display, surface);
    }
}
