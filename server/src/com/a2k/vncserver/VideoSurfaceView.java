package com.a2k.vncserver;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.Surface;

import com.a2k.vncserver.VncJni;

class VideoSurfaceView extends GLSurfaceView {
    private static final String TAG = "VideoSurfaceView";

    VideoRender mRenderer;

	private VncJni m_VncJni = null;

    public VideoSurfaceView(Context context, VncJni vncJni) {
        super(context);

        setEGLContextClientVersion(2);
        m_VncJni = vncJni;
        mRenderer = new VideoRender(context, this);
        super.setEGLConfigChooser(8 , 8, 8, 8, 16, 0);
        setRenderer(mRenderer);
    }

    @Override
    public void onResume() {
        queueEvent(new Runnable(){
                public void run() {
                	mRenderer.setVncJni(m_VncJni);
                }});

        super.onResume();
    }

    private OnSurfaceCreatedListener onSurfaceCreatedListener;
    public interface OnSurfaceCreatedListener
    {
        void onSurfaceCreated(Surface surface);
    }
    
    public void setOnSurfaceCreatedListener(OnSurfaceCreatedListener listener) {
    	onSurfaceCreatedListener = listener;
    }

    public void onSurfaceCreated(Surface surface) {
    	if (onSurfaceCreatedListener != null)
    	{
    		onSurfaceCreatedListener.onSurfaceCreated(surface);
    	}
    }

    /**
     * A GLSurfaceView implementation that wraps TextureRender.  Used to render frames from a
     * video decoder to a View.
     */
    private static class VideoRender
            implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
        private static String TAG = "VideoRender";

        private TextureRender mTextureRender;
        private Surface mSurface;
        private int mWidth;
        private int mHeight;
        private SurfaceTexture mSurfaceTexture;
        private boolean updateSurface = false;
        
        private VideoSurfaceView m_Parent;
        private long m_GraphicBuffer;

        private VncJni m_VncJni;

        public VideoRender(Context context, VideoSurfaceView parent) {
            m_Parent = parent;
            mWidth = 1024;
            mHeight = 1024;
            mTextureRender = new TextureRender();
        }

        public void setVncJni(VncJni vncJni) {
            m_VncJni = vncJni;
        }

        public void onDrawFrame(GL10 glUnused) {
            synchronized(this) {
                if (updateSurface) {
                    mSurfaceTexture.updateTexImage();
                    m_VncJni.glOnFrameAvailable(m_GraphicBuffer);
                    updateSurface = false;
                }
            }

            mTextureRender.drawFrame(mSurfaceTexture);
        }

        public void onSurfaceChanged(GL10 glUnused, int width, int height) {
        	Log.d(TAG, "onSurfaceChanged");
        }

        public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
        	Log.d(TAG, "onSurfaceCreated");
            mTextureRender.surfaceCreated();
            m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(mWidth, mHeight);
            m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);
            /*
             * Create the SurfaceTexture that will feed this textureID,
             * and pass it to the MediaPlayer
             */
            mSurfaceTexture = new SurfaceTexture(mTextureRender.getTextureId());
            mSurfaceTexture.setOnFrameAvailableListener(this);
            mSurfaceTexture.setDefaultBufferSize(mWidth, mHeight);
            mSurface = new Surface(mSurfaceTexture);
            m_Parent.onSurfaceCreated(mSurface);

            synchronized(this) {
                updateSurface = false;
            }
        }

        synchronized public void onFrameAvailable(SurfaceTexture surface) {
            Log.d(TAG, "onFrameAvailable");
            updateSurface = true;
        }
    }  // End of class VideoRender.

}  // End of class VideoSurfaceView.
