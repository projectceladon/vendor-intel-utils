package com.intel.multicamera;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import java.util.Arrays;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class MainActivity extends Activity {
  private String TAG = "multicamera";
  private Size previewsize,previewsize1,previewsize2,previewsize3;
  private int preview_width =640;
  private int preview_height =480;
  private Semaphore mCameraOpenCloseLock = new Semaphore(1);
  private int vwidth;
  private int vheight;
  private static final int PERMISSIONS_REQUEST_CAMERA = 1;
  private Size jpegSizes[] = null;

  private TextureView textureView,textureView1,textureView2,textureView3;
  private CameraDevice cameraDevice,cameraDevice1,cameraDevice2,cameraDevice3;
  private CaptureRequest.Builder previewBuilder,previewBuilder1,previewBuilder2,previewBuilder3;
  private CameraCaptureSession previewSession,previewSession1,previewSession2,previewSession3;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);

        LinearLayout layout = findViewById(R.id.lay1);
        ViewGroup.LayoutParams params1 = layout.getLayoutParams();

        params1.height = displayMetrics.heightPixels;
        params1.width = displayMetrics.widthPixels;
        layout.setLayoutParams(params1);
        Log.d(TAG,"params1.height: "+params1.height+" params1.width: "+ params1.width);


        vheight = displayMetrics.heightPixels/2;
        vwidth = displayMetrics.widthPixels/2;

        Log.d(TAG,"vwidth1: "+vwidth+" vheight1: "+vheight);


        android.widget.FrameLayout.LayoutParams params = new android.widget.FrameLayout.LayoutParams(vwidth, vheight);

        textureView = (TextureView) findViewById(R.id.textureview1);
        textureView1 = (TextureView) findViewById(R.id.textureview2);
		    textureView2 = (TextureView) findViewById(R.id.textureview3);
		    textureView3 = (TextureView) findViewById(R.id.textureview4);

        textureView.setSurfaceTextureListener(surfaceTextureListener);
        textureView1.setSurfaceTextureListener(surfaceTextureListener1);
		    textureView2.setSurfaceTextureListener(surfaceTextureListener2);
		    textureView3.setSurfaceTextureListener(surfaceTextureListener3);

		    textureView.setLayoutParams(params);
        textureView1.setLayoutParams(params);
        textureView2.setLayoutParams(params);
		    textureView3.setLayoutParams(params);

    }

    
    @Override
    public void onRequestPermissionsResult (int requestCode, String[] permissions,
            int[] grantResults) {
        if (requestCode == PERMISSIONS_REQUEST_CAMERA) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openCamera();
            } else {
                finish();
            }
        } 

    }



        private TextureView.SurfaceTextureListener surfaceTextureListener = new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                Log.d(TAG,"Inside surfaceTextureListener onSurfaceTextureAvailable:");
                openCamera();
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        };


        private TextureView.SurfaceTextureListener surfaceTextureListener1 = new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                Log.d(TAG,"Inside surfaceTextureListener1 onSurfaceTextureAvailable:");
                openCamera1();
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        };

    	private TextureView.SurfaceTextureListener surfaceTextureListener2 = new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                Log.d(TAG,"Inside surfaceTextureListener2 onSurfaceTextureAvailable:");
                openCamera2();
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        };

    	private TextureView.SurfaceTextureListener surfaceTextureListener3 = new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                Log.d(TAG,"Inside surfaceTextureListener3 onSurfaceTextureAvailable:");
                openCamera3();
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                return false;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {

            }
        };

        public void openCamera() {
            CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);

            try {
                Log.d(TAG,"Inside openCamera() Total Cameras: "+manager.getCameraIdList().length);
                String camerId = manager.getCameraIdList()[0];
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                int total_psizes = map.getOutputSizes(ImageFormat.JPEG).length;
                Log.d(TAG,"openCamera() total_psizes: "+total_psizes);

                for (int i=0;i<total_psizes;i++){
                    Size psize = map.getOutputSizes(SurfaceTexture.class)[i];
                    Log.d(TAG,"camera1 preview width: "+psize.getWidth()+" preview width: "+psize.getHeight());
                    long minframe = map.getOutputMinFrameDuration(SurfaceTexture.class,psize);
                    Log.d(TAG,"camera1 preview MinFrameDuration: "+minframe);
                    previewsize = psize;
                    /*
                    if(minframe <= 40000000){
                        previewsize = psize;
                        break;
                    }*/
                }
                Log.d(TAG,"camera1 final preview width:"+previewsize.getWidth()+" final preview height: "+previewsize.getHeight() );

                configureTransform(vwidth, vheight);
                if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Consider calling
                    //    Activity#requestPermissions
                    // here to request the missing permissions, and then overriding
                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                    //                                          int[] grantResults)
                    // to handle the case where the user grants the permission. See the documentation
                    // for Activity#requestPermissions for more details.
                    requestPermissions(new String[] {Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
                    return;
                }
                /*
                if (!mCameraOpenCloseLock.tryAcquire(2500, TimeUnit.MILLISECONDS)) {
                    throw new RuntimeException("Time out waiting to lock camera opening.");
                }*/
                manager.openCamera(camerId, stateCallback, null);
            }catch (Exception e)
            {

            }
        }

        public void openCamera1() {
            Log.d(TAG,"Inside openCamera1()");
            CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
            try {
                String camerId = manager.getCameraIdList()[1];
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                previewsize1 = map.getOutputSizes(SurfaceTexture.class)[0];

                configureTransform1(vwidth, vheight);
                if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Consider calling
                    //    Activity#requestPermissions
                    // here to request the missing permissions, and then overriding
                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                    //                                          int[] grantResults)
                    // to handle the case where the user grants the permission. See the documentation
                    // for Activity#requestPermissions for more details.
                    //requestPermissions(new String[] {Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
                    return;
                }
                manager.openCamera(camerId, stateCallback1, null);
            }catch (Exception e)
            {

            }
        }

    	public void openCamera2() {
            Log.d(TAG,"Inside openCamera2()");
            CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
            try {
                String camerId = manager.getCameraIdList()[2];
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                previewsize2 = map.getOutputSizes(SurfaceTexture.class)[0];

                configureTransform2(vwidth, vheight);
                if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Consider calling
                    //    Activity#requestPermissions
                    // here to request the missing permissions, and then overriding
                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                    //                                          int[] grantResults)
                    // to handle the case where the user grants the permission. See the documentation
                    // for Activity#requestPermissions for more details.
                    //requestPermissions(new String[] {Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
                    return;
                }
                manager.openCamera(camerId, stateCallback2, null);
            }catch (Exception e)
            {

            }
        }

    	public void openCamera3() {
            Log.d(TAG,"Inside openCamera3()");
            CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
            try {
                String camerId = manager.getCameraIdList()[3];
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(camerId);
                StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                previewsize3 = map.getOutputSizes(SurfaceTexture.class)[0];

                configureTransform3(vwidth, vheight);
                if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                    // TODO: Consider calling
                    //    Activity#requestPermissions
                    // here to request the missing permissions, and then overriding
                    //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                    //                                          int[] grantResults)
                    // to handle the case where the user grants the permission. See the documentation
                    // for Activity#requestPermissions for more details.
                    //requestPermissions(new String[] {Manifest.permission.CAMERA}, PERMISSIONS_REQUEST_CAMERA);
                    return;
                }
                manager.openCamera(camerId, stateCallback3, null);
            }catch (Exception e)
            {

            }
        }

        private CameraDevice.StateCallback stateCallback=new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
    			Log.d(TAG,"inside stateCallback onOpened()");
                cameraDevice=camera;
                startCamera();
            }

            @Override
            public void onDisconnected(CameraDevice camera) {

            }

            @Override
            public void onError(CameraDevice camera, int error) {

            }
        };

        private CameraDevice.StateCallback stateCallback1=new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
    			Log.d(TAG,"inside stateCallback1 onOpened()");
                cameraDevice1=camera;
                startCamera1();
            }

            @Override
            public void onDisconnected(CameraDevice camera) {

            }

            @Override
            public void onError(CameraDevice camera, int error) {

            }
        };

    	private CameraDevice.StateCallback stateCallback2=new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
    			Log.d(TAG,"inside stateCallback2 onOpened()");
                cameraDevice2=camera;
                startCamera2();
            }

            @Override
            public void onDisconnected(CameraDevice camera) {

            }

            @Override
            public void onError(CameraDevice camera, int error) {

            }
        };

    	private CameraDevice.StateCallback stateCallback3=new CameraDevice.StateCallback() {
            @Override
            public void onOpened(CameraDevice camera) {
    			Log.d(TAG,"inside stateCallback3 onOpened()");
                cameraDevice3=camera;
                startCamera3();
            }

            @Override
            public void onDisconnected(CameraDevice camera) {

            }

            @Override
            public void onError(CameraDevice camera, int error) {

            }
        };

        void  startCamera()
        {
    		Log.d(TAG,"inside startCamera()");
            if(cameraDevice==null||!textureView.isAvailable()|| previewsize==null)
            {
                return;
            }

            SurfaceTexture texture=textureView.getSurfaceTexture();
            if(texture==null)
            {
                return;
            }
            texture.setDefaultBufferSize(preview_width,preview_height);
            //texture.setDefaultBufferSize(previewsize.getWidth(),previewsize.getHeight());
            Log.d(TAG,"previewsize width: "+previewsize.getWidth()+" previewsize height: "+previewsize.getHeight());
            Surface surface=new Surface(texture);

            try
            {
                previewBuilder=cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);

            }catch (Exception e)
            {

            }
            previewBuilder.addTarget(surface);

            try
            {
                cameraDevice.createCaptureSession(Arrays.asList(surface), new CameraCaptureSession.StateCallback() {
                    @Override
                    public void onConfigured(CameraCaptureSession session) {
                        previewSession=session;
                        getChangedPreview();
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession session) {

                    }
                },null);
            }catch (Exception e)
            {

            }
        }


        void  startCamera1()
        {
    		Log.d(TAG,"inside startCamera1()");
            if(cameraDevice1==null||!textureView1.isAvailable()|| previewsize1==null)
            {
                return;
            }

            SurfaceTexture texture1=textureView1.getSurfaceTexture();
            if(texture1==null)
            {
                return;
            }
            texture1.setDefaultBufferSize(preview_width,preview_height);
            //texture1.setDefaultBufferSize(previewsize1.getWidth(),previewsize1.getHeight());

            Surface surface1=new Surface(texture1);

            try
            {
                previewBuilder1=cameraDevice1.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            }catch (Exception e)
            {
            }
            previewBuilder1.addTarget(surface1);
            try
            {
                cameraDevice1.createCaptureSession(Arrays.asList(surface1), new CameraCaptureSession.StateCallback() {
                    @Override
                    public void onConfigured(CameraCaptureSession session) {
                        previewSession1=session;
                        getChangedPreview1();
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession session) {

                    }
                },null);
            }catch (Exception e)
            {

            }
        }

    	void  startCamera2()
        {
    		Log.d(TAG,"inside startCamera2()");
            if(cameraDevice2 ==null||!textureView2.isAvailable()|| previewsize2==null)
            {
                return;
            }

            SurfaceTexture texture2=textureView2.getSurfaceTexture();
            if(texture2==null)
            {
                return;
            }
            texture2.setDefaultBufferSize(preview_width,preview_height);
            //texture2.setDefaultBufferSize(previewsize1.getWidth(),previewsize1.getHeight());

            Surface surface2=new Surface(texture2);

            try
            {
                previewBuilder2=cameraDevice2.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            }catch (Exception e)
            {
            }
            previewBuilder2.addTarget(surface2);
            try
            {
                cameraDevice2.createCaptureSession(Arrays.asList(surface2), new CameraCaptureSession.StateCallback() {
                    @Override
                    public void onConfigured(CameraCaptureSession session) {
                        previewSession2=session;
                        getChangedPreview2();
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession session) {

                    }
                },null);
            }catch (Exception e)
            {

            }
        }

    	void  startCamera3()
        {
    		Log.d(TAG,"inside startCamera3()");
            if(cameraDevice3 ==null||!textureView3.isAvailable()|| previewsize3==null)
            {
                return;
            }

            SurfaceTexture texture3=textureView3.getSurfaceTexture();
            if(texture3==null)
            {
                return;
            }
            texture3.setDefaultBufferSize(preview_width,preview_height);
            //texture3.setDefaultBufferSize(previewsize1.getWidth(),previewsize1.getHeight());

            Surface surface3=new Surface(texture3);

            try
            {
                previewBuilder3=cameraDevice3.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            }catch (Exception e)
            {
            }
            previewBuilder3.addTarget(surface3);
            try
            {
                cameraDevice3.createCaptureSession(Arrays.asList(surface3), new CameraCaptureSession.StateCallback() {
                    @Override
                    public void onConfigured(CameraCaptureSession session) {
                        previewSession3=session;
                        getChangedPreview3();
                    }

                    @Override
                    public void onConfigureFailed(CameraCaptureSession session) {

                    }
                },null);
            }catch (Exception e)
            {

            }
        }

        void getChangedPreview()
        {
            if(cameraDevice==null)
            {
                return;
            }

            previewBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            HandlerThread thread=new HandlerThread("changed Preview");
            thread.start();
            Handler handler=new Handler(thread.getLooper());
            try
            {
                previewSession.setRepeatingRequest(previewBuilder.build(), null, handler);
            }catch (Exception e){}
        }

        void getChangedPreview1()
        {
            if(cameraDevice1==null)
            {
                return;
            }
            previewBuilder1.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            HandlerThread thread1=new HandlerThread("changed Preview");
            thread1.start();
            Handler handler1=new Handler(thread1.getLooper());
            try
            {
                previewSession1.setRepeatingRequest(previewBuilder1.build(), null, handler1);
            }catch (Exception e){}
        }

    	void getChangedPreview2()
        {
            if(cameraDevice2==null)
            {
                return;
            }
            previewBuilder2.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            HandlerThread thread2=new HandlerThread("changed Preview");
            thread2.start();
            Handler handler2=new Handler(thread2.getLooper());
            try
            {
                previewSession2.setRepeatingRequest(previewBuilder2.build(), null, handler2);
            }catch (Exception e){}
        }

    	void getChangedPreview3()
        {
            if(cameraDevice3==null)
            {
                return;
            }
            previewBuilder3.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
            HandlerThread thread3=new HandlerThread("changed Preview");
            thread3.start();
            Handler handler3=new Handler(thread3.getLooper());
            try
            {
                previewSession3.setRepeatingRequest(previewBuilder3.build(), null, handler3);
            }catch (Exception e){}
        }


        private void configureTransform(int viewWidth, int viewHeight) {

            if (null == textureView || null == previewsize) {
                return;
            }
            int rotation = this.getWindowManager().getDefaultDisplay().getRotation();
            Matrix matrix = new Matrix();
            RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
            Log.d(TAG," configureTransform() viewWidth: "+viewWidth+" viewHeight: "+viewHeight);
            RectF bufferRect = new RectF(0, 0, preview_height, preview_width);
            float centerX = viewRect.centerX();
            float centerY = viewRect.centerY();
            if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
                bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
                matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
                float scale = Math.max(
                        (float) viewHeight / preview_height,
                        (float) viewWidth / preview_width);
                matrix.postScale(scale, scale, centerX, centerY);
                matrix.postRotate(90 * (rotation - 2), centerX, centerY);
            } else if (Surface.ROTATION_180 == rotation) {
                matrix.postRotate(180, centerX, centerY);
            }
            textureView.setTransform(matrix);
        }

        private void configureTransform1(int viewWidth, int viewHeight) {

            if (null == textureView1 || null == previewsize1) {
                return;
            }
            int rotation = this.getWindowManager().getDefaultDisplay().getRotation();
            Matrix matrix = new Matrix();
            RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
            Log.d(TAG," configureTransform1() viewWidth: "+viewWidth+" viewHeight: "+viewHeight);
            RectF bufferRect = new RectF(0, 0, preview_height, preview_width);
            float centerX = viewRect.centerX();
            float centerY = viewRect.centerY();
            if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
                bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
                matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
                float scale = Math.max(
                        (float) viewHeight / preview_height,
                        (float) viewWidth / preview_width);
                matrix.postScale(scale, scale, centerX, centerY);
                matrix.postRotate(90 * (rotation - 2), centerX, centerY);
            } else if (Surface.ROTATION_180 == rotation) {
                matrix.postRotate(180, centerX, centerY);
            }
            textureView1.setTransform(matrix);
        }

    	private void configureTransform2(int viewWidth, int viewHeight) {

            if (null == textureView2 || null == previewsize2) {
                return;
            }
            int rotation = this.getWindowManager().getDefaultDisplay().getRotation();
            Matrix matrix = new Matrix();
            RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
            Log.d(TAG," configureTransform2() viewWidth: "+viewWidth+" viewHeight: "+viewHeight);
            RectF bufferRect = new RectF(0, 0, preview_height, preview_width);
            float centerX = viewRect.centerX();
            float centerY = viewRect.centerY();
            if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
                bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
                matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
                float scale = Math.max(
                        (float) viewHeight / preview_height,
                        (float) viewWidth / preview_width);
                matrix.postScale(scale, scale, centerX, centerY);
                matrix.postRotate(90 * (rotation - 2), centerX, centerY);
            } else if (Surface.ROTATION_180 == rotation) {
                matrix.postRotate(180, centerX, centerY);
            }
            textureView2.setTransform(matrix);
        }

    	private void configureTransform3(int viewWidth, int viewHeight) {

            if (null == textureView3 || null == previewsize3) {
                return;
            }
            int rotation = this.getWindowManager().getDefaultDisplay().getRotation();
            Matrix matrix = new Matrix();
            RectF viewRect = new RectF(0, 0, viewWidth, viewHeight);
            Log.d(TAG," configureTransform2() viewWidth: "+viewWidth+" viewHeight: "+viewHeight);
            RectF bufferRect = new RectF(0, 0, preview_height, preview_width);
            float centerX = viewRect.centerX();
            float centerY = viewRect.centerY();
            if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
                bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
                matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
                float scale = Math.max(
                        (float) viewHeight / preview_height,
                        (float) viewWidth / preview_width);
                matrix.postScale(scale, scale, centerX, centerY);
                matrix.postRotate(90 * (rotation - 2), centerX, centerY);
            } else if (Surface.ROTATION_180 == rotation) {
                matrix.postRotate(180, centerX, centerY);
            }
            textureView3.setTransform(matrix);
        }

        @Override
        public void onPause() {
            //closeCamera();
            //closeCamera1();
            //closeCamera2();
            //closeCamera3();
            //stopBackgroundThread();
            super.onPause();
        }

        /*
        private void closeCamera() {
            try {
                mCameraOpenCloseLock.acquire();
                if (null != previewSession) {
                    previewSession.close();
                    previewSession = null;
                }
                if (null != cameraDevice) {
                    cameraDevice.close();
                    cameraDevice = null;
                }
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted while trying to lock camera closing.", e);
            } finally {
                mCameraOpenCloseLock.release();
            }
        }*/

}
