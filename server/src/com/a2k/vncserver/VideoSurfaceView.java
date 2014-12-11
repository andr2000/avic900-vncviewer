package com.a2k.vncserver;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.SurfaceHolder;

class VideoSurfaceView extends GLSurfaceView implements
	SurfaceTexture.OnFrameAvailableListener
{
	private static final String TAG = "VideoSurfaceView";

	private TextureRender m_TextureRender;
	private SurfaceTexture m_SurfaceTexture;

	private int m_Width;
	private int m_Height;

	private VideoSurfaceViewListener m_Listener;

	public VideoSurfaceView(Context context, VncJni vncJni, int width, int height)
	{
		super(context);
		m_Width = width;
		m_Height = height;
		m_TextureRender = new TextureRender(vncJni, width, height);
	}

	@Override
	public void onPause()
	{
		Log.d(TAG, "onPause");
	}

	@Override
	public void onResume()
	{
		Log.d(TAG, "onResume");
	}

	@Override
	public void queueEvent(Runnable r)
	{
		Log.d(TAG, "surfaceCreated");
	}

	public static interface VideoSurfaceViewListener
	{
		public void onSurfaceTextureCreated(SurfaceTexture surfaceTexture);
	}

	public void setVideoSurfaceViewListener(VideoSurfaceViewListener listener)
	{
		m_Listener = listener;
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder)
	{
		Log.d(TAG, "surfaceCreated surface " + holder.getSurface());
		m_TextureRender.surfaceCreated(holder.getSurface());
		m_SurfaceTexture = new SurfaceTexture(m_TextureRender.getTextureId());
		m_SurfaceTexture.setDefaultBufferSize(m_Width, m_Height);
		m_SurfaceTexture.setOnFrameAvailableListener(this);
		if (m_Listener != null)
		{
			m_Listener.onSurfaceTextureCreated(m_SurfaceTexture);
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		Log.d(TAG, "surfaceDestroyed surface " + holder.getSurface());
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		Log.d(TAG, "surfaceChanged holder " + holder.getSurface() +
			" format " + format + " width " + w + " height " + h);
	}

	public void onFrameAvailable(SurfaceTexture surfaceTexture)
	{
		Log.d(TAG, "onFrameAvailable");
		m_SurfaceTexture.updateTexImage();
		m_TextureRender.drawFrame(m_SurfaceTexture);
		m_TextureRender.swapBuffers();
	}
}
