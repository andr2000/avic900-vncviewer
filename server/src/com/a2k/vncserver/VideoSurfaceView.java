package com.a2k.vncserver;

import android.content.Context;
import android.opengl.GLSurfaceView;

class VideoSurfaceView extends GLSurfaceView
{
	private static final String TAG = "VideoSurfaceView";

	private Renderer m_Renderer;

	public VideoSurfaceView(Context context, Renderer renderer)
	{
		super(context);
		setEGLContextClientVersion(2);
		super.setEGLConfigChooser(8 , 8, 8, 8, 16, 0);
		m_Renderer = renderer;
		setRenderer(m_Renderer);
	}
}
