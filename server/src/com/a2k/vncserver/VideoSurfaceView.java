package com.a2k.vncserver;

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.util.Log;
import android.view.Surface;

import com.a2k.vncserver.VncJni;

class VideoSurfaceView extends GLSurfaceView {
    private static final String TAG = "VideoSurfaceView";
    private static final int SLEEP_TIME_MS = 1000;

    VideoRender mRenderer;
    private MediaPlayer mMediaPlayer = null;

	private long m_GraphicBuffer;
	private VncJni m_VncJni = null;

    public VideoSurfaceView(Context context, MediaPlayer mp, VncJni vncJni) {
        super(context);

        setEGLContextClientVersion(2);
        mMediaPlayer = mp;
        m_VncJni = vncJni;
        mRenderer = new VideoRender(context);
        super.setEGLConfigChooser(8 , 8, 8, 8, 16, 0);
        setRenderer(mRenderer);
    }

    @Override
    public void onResume() {
        queueEvent(new Runnable(){
                public void run() {
                	mRenderer.setVncJni(m_VncJni);
                    mRenderer.setMediaPlayer(mMediaPlayer);
                }});

        super.onResume();
    }

    public void startTest() throws Exception {
        Thread.sleep(SLEEP_TIME_MS);
        mMediaPlayer.start();

        Thread.sleep(SLEEP_TIME_MS * 5);
        mMediaPlayer.setSurface(null);

        while (mMediaPlayer.isPlaying()) {
            Thread.sleep(SLEEP_TIME_MS);
        }
    }

    public Surface getSurface()
    {
    	return mRenderer.getSurface();
    }

    public void setSurfaceSize(int width, int height)
    {
        mRenderer.setSurfaceSize(width, height);
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
        private SurfaceTexture mSurfaceTexture;
        private boolean updateSurface = false;

        private MediaPlayer mMediaPlayer;
    	private VncJni m_VncJni;

        public VideoRender(Context context) {
            mTextureRender = new TextureRender();
        }

        public void setMediaPlayer(MediaPlayer player) {
            mMediaPlayer = player;
        }

        public void setVncJni(VncJni vncJni) {
            m_VncJni = vncJni;
        }

        public Surface getSurface() {
            return mSurface;
        }

        public void setSurfaceSize(int width, int height)
        {
            mSurfaceTexture.setDefaultBufferSize(width, height);
        }

        public void onDrawFrame(GL10 glUnused) {
            synchronized(this) {
                if (updateSurface) {
                    mSurfaceTexture.updateTexImage();
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

            /*
             * Create the SurfaceTexture that will feed this textureID,
             * and pass it to the MediaPlayer
             */
            mSurfaceTexture = new SurfaceTexture(mTextureRender.getTextureId());
            mSurfaceTexture.setOnFrameAvailableListener(this);

            mSurface = new Surface(mSurfaceTexture);
            //mMediaPlayer.setSurface(mSurface);
            //mSurface.release();

            /*
            try {
                mMediaPlayer.prepare();
            } catch (IOException t) {
                Log.e(TAG, "media player prepare failed");
            }
            */

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
