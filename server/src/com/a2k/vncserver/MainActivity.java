package com.a2k.vncserver;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaPlayer;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;

import java.io.IOException;
import java.io.FileOutputStream;
import java.lang.System;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.a2k.vncserver.Gles;
import com.a2k.vncserver.VncJni;

public class MainActivity extends Activity implements TextureView.SurfaceTextureListener,
	SurfaceTexture.OnFrameAvailableListener, Runnable
{
	public static final String TAG = "MainActivity";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private Surface m_Surface;
	private SurfaceTexture m_SurfaceTexture;
	private TextureView m_TextureView;
	private final Object m_FrameAvailableLock = new Object();
	private boolean m_FrameAvailable = false;
	private boolean m_Running;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private long m_GraphicBuffer;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;

	private VncJni m_VncJni = new VncJni();
	private Gles m_Gles = new Gles();
	private Object m_ThreadStarted = new Object();

	private MediaPlayer m_MediaPlayer;

	private int m_FrameCounter = 0;
	private int m_UpdateCounter = 0;
	private int m_DrawCounter = 0;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Log.d(TAG, m_VncJni.protoGetVersion());
		
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		m_ScreenDensity = metrics.densityDpi;

		m_ProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);

		m_TextureView = new TextureView(this);
		m_TextureView.setSurfaceTextureListener(this);

		addContentView(m_TextureView, new FrameLayout.LayoutParams(
			FrameLayout.LayoutParams.WRAP_CONTENT,
			/*FrameLayout.LayoutParams.WRAP_CONTENT*/ 700,
			Gravity.BOTTOM
			)
		);

		m_ButtonStartStop = (Button)findViewById(R.id.buttonStartStop);
		m_ButtonStartStop.setEnabled(false);
		m_ButtonStartStop.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				if (m_ProjectionStarted)
				{
					stopScreenSharing();
					m_ButtonStartStop.setText("Start");
				}
				else
				{
					m_ButtonStartStop.setText("Stop");
					shareScreen();
				}
				m_ProjectionStarted ^= true;
			}
		});
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		if (m_MediaProjection != null)
		{
			m_MediaProjection.stop();
			m_MediaProjection = null;
		}
	}

	private void startPlayer()
	{
		m_MediaPlayer = new MediaPlayer();

		try
		{
			AssetFileDescriptor afd = getAssets().openFd("big_buck_bunny.mp4");
			m_MediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
			m_MediaPlayer.setSurface(m_Surface);
			m_MediaPlayer.setLooping(true);
			m_MediaPlayer.prepare();
			m_MediaPlayer.start();
		}
		catch (IOException e)
		{
			throw new RuntimeException("Could not open input video!");
		}
	}

	@Override
	public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height)
	{
		Log.d(TAG, "onSurfaceTextureAvailable");
		m_DisplayWidth = width;
		m_DisplayHeight = height;

		m_Running = true;
		Thread thrd = new Thread(this);
		thrd.start();
		synchronized(m_ThreadStarted)
		{
			try
			{
				m_ThreadStarted.wait();
			}
			catch(InterruptedException e)
			{
			}
		}

		startPlayer();

		m_ButtonStartStop.setText("Start");
		m_ButtonStartStop.setEnabled(true);
	}

	@Override
	public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height)
	{
		Log.d(TAG, "onSurfaceTextureSizeChanged");
	}

	@Override
	public boolean onSurfaceTextureDestroyed(SurfaceTexture surface)
	{
		Log.d(TAG, "onSurfaceTextureDestroyed");
		return false;
	}

	@Override
	public void onSurfaceTextureUpdated(SurfaceTexture surface)
	{
		m_UpdateCounter++;
		Log.d(TAG, "onSurfaceTextureUpdated " + m_UpdateCounter);
	}

	@Override
	public void onFrameAvailable(SurfaceTexture surfaceTexture)
	{
		m_FrameCounter++;
		Log.d(TAG, "onFrameAvailable " + m_FrameCounter);
		synchronized(m_FrameAvailableLock)
		{
			m_FrameAvailable = true;
			m_FrameAvailableLock.notify();
		}
	}

	@Override
	public void run()
	{
		m_Gles.initGL(m_TextureView.getSurfaceTexture());
		m_Gles.setupVertexBuffer();
		m_Gles.setupTexture(m_DisplayWidth, m_DisplayHeight);
		int texture = m_Gles.getTexture();

		m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(m_DisplayWidth, m_DisplayHeight);
		m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);

		m_Gles.loadShaders();

		m_SurfaceTexture = new SurfaceTexture(texture);
		m_SurfaceTexture.setOnFrameAvailableListener(this);
		m_Surface = new Surface(m_SurfaceTexture);
		Log.d(TAG, "GLES initialized");

		synchronized(m_ThreadStarted)
		{
			m_ThreadStarted.notifyAll();
		}
		float[] videoTextureTransform = new float[16];
		m_Gles.setViewport(m_TextureView.getWidth(), m_TextureView.getHeight());
		while (m_Running)
		{
			synchronized (m_FrameAvailableLock)
			{
				while (!m_FrameAvailable)
				{
					try
					{
						m_FrameAvailableLock.wait();
					}
					catch (InterruptedException e)
					{
					}
				}
				m_FrameAvailable = false;
			}
			m_DrawCounter++;
			Log.d(TAG, "draw " + m_DrawCounter);
			m_SurfaceTexture.updateTexImage();
			m_SurfaceTexture.getTransformMatrix(videoTextureTransform);
			m_Gles.draw(videoTextureTransform);
			m_Gles.swapBufers();
		}
//		deinitGLComponents();
//		deinitGL();
	}

	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (requestCode != PERMISSION_CODE)
		{
			Log.e(TAG, "Unknown request code: " + requestCode);
			return;
		}
		if (resultCode != RESULT_OK) {
			Toast.makeText(this, "User denied screen sharing permission",
				Toast.LENGTH_SHORT).show();
			return;
		}
		m_MediaProjection = m_ProjectionManager.getMediaProjection(resultCode, data);
		m_VirtualDisplay = createVirtualDisplay();
	}

	private void shareScreen()
	{
		if (m_Surface == null)
		{
			return;
		}
		if (m_MediaProjection == null)
		{
			startActivityForResult(m_ProjectionManager.createScreenCaptureIntent(), PERMISSION_CODE);
		}
		else
		{
			m_VirtualDisplay = createVirtualDisplay();
		}
	}

	private void stopScreenSharing()
	{
		if (m_VirtualDisplay != null)
		{
			m_VirtualDisplay.release();
			m_VirtualDisplay = null;
		}
	}

	private VirtualDisplay createVirtualDisplay()
	{
		return m_MediaProjection.createVirtualDisplay("vncserver",
			m_DisplayWidth, m_DisplayHeight, m_ScreenDensity,
			DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
			m_Surface, null /*Callbacks*/, null /*Handler*/);
	}

	private void resizeVirtualDisplay()
	{
		if (m_VirtualDisplay != null)
		{
			m_VirtualDisplay.resize(m_DisplayWidth, m_DisplayHeight, m_ScreenDensity);
		}
	}

	private class MediaProjectionCallback extends MediaProjection.Callback
	{
		@Override
		public void onStop()
		{
			m_MediaProjection = null;
			stopScreenSharing();
		}
	}
}
