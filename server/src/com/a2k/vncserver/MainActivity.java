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
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;

import java.io.IOException;
import java.io.FileOutputStream;
import java.lang.System;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import com.a2k.vncserver.VncJni;
import com.a2k.vncserver.VideoSurfaceView;

public class MainActivity extends Activity
{
	public static final String TAG = "MainActivity";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private Surface m_Surface;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;

	private VncJni m_VncJni = new VncJni();
	
	private VideoSurfaceView m_VideoSurfaceView;

	private MediaPlayer m_MediaPlayer;

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

		m_MediaPlayer = new MediaPlayer();
		try
		{
			AssetFileDescriptor afd = getAssets().openFd("big_buck_bunny.mp4");
			m_MediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
			m_MediaPlayer.setLooping(true);
		}
		catch (IOException e)
		{
			throw new RuntimeException("Could not open input video!");
		}
		m_VideoSurfaceView = new VideoSurfaceView(this, m_MediaPlayer, m_VncJni);

		//setContentView(m_VideoSurfaceView);
		addContentView(m_VideoSurfaceView, new FrameLayout.LayoutParams(
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
					//m_MediaPlayer.stop();
					m_ButtonStartStop.setText("Start");
				}
				else
				{
					m_ButtonStartStop.setText("Stop");
					m_DisplayWidth = m_VideoSurfaceView.getWidth();
					m_DisplayHeight = m_VideoSurfaceView.getHeight();
					m_VideoSurfaceView.setSurfaceSize(m_DisplayWidth, m_DisplayHeight);
					m_Surface = m_VideoSurfaceView.getSurface();
					shareScreen();
					//m_MediaPlayer.start();
				}
				m_ProjectionStarted ^= true;
			}
		});
		
		m_ButtonStartStop.setText("Start");
		m_ButtonStartStop.setEnabled(true);
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

	@Override
	protected void onResume() {
		super.onResume();
		m_VideoSurfaceView.onResume();
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
}
