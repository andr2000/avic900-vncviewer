package com.a2k.vncserver;

import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class Renderer implements GLSurfaceView.Renderer
{
	private static String TAG = "Renderer";

	private int m_Width;
	private int m_Height;
	private SurfaceTexture m_SurfaceTexture;

	private TextureRender m_TextureRender;
	private long m_GraphicBuffer;

	private VncJni m_VncJni;

	public Renderer(VncJni vncJni, int width, int height) {
		m_VncJni = vncJni;
		m_Width = width;
		m_Height = height;
		m_TextureRender = new TextureRender();
	}

	private RendererListener m_Listener;

	public static interface RendererListener
	{
		public void onSurfaceTextureCreated(SurfaceTexture surfaceTexture);
		public void onDrawDone();
	}

	public void setRendererListener(RendererListener listener)
	{
		m_Listener = listener;
	}

	public void onDrawFrame(GL10 glUnused)
	{
		Log.d(TAG, "onDrawFrame");
		m_SurfaceTexture.updateTexImage();
		m_TextureRender.drawFrame(m_SurfaceTexture);
		//m_VncJni.glOnFrameAvailable(m_GraphicBuffer);
		if (m_Listener != null)
		{
			m_Listener.onDrawDone();
		}
	}

	public void onSurfaceChanged(GL10 glUnused, int width, int height) {
		Log.d(TAG, "onSurfaceChanged");
	}

	public void onSurfaceCreated(GL10 glUnused, EGLConfig config)
	{
		Log.d(TAG, "onSurfaceCreated");
		m_TextureRender.surfaceCreated();
		//m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(mWidth, mHeight);
		//m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);
		m_SurfaceTexture = new SurfaceTexture(m_TextureRender.getTextureId());
		m_SurfaceTexture.setDefaultBufferSize(m_Width, m_Height);
		if (m_Listener != null)
		{
			m_Listener.onSurfaceTextureCreated(m_SurfaceTexture);
		}
	}
}
