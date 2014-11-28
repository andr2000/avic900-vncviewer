package com.a2k.vncserver;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.content.Intent;

import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.hardware.display.DisplayManager;
import android.util.DisplayMetrics;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;

import android.widget.Toast;
import android.util.Log;

public class VncProjectionActivity extends Activity
{
	private static final String TAG = "vncserver";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private boolean m_ScreenSharing = true;
	private Surface m_Surface;
	private SurfaceView m_SurfaceView;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_vnc_projection);

		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		m_ScreenDensity = metrics.densityDpi;

		m_SurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
		m_Surface = m_SurfaceView.getHolder().getSurface();

		m_ProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);
		shareScreen();
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
	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (requestCode != PERMISSION_CODE)
		{
			Log.e(TAG, "Unknown request code: " + requestCode);
			return;
		}
		if (resultCode != RESULT_OK) {
			Toast.makeText(this,
				"User denied screen sharing permission", Toast.LENGTH_SHORT).show();
			return;
		}
		m_MediaProjection = m_ProjectionManager.getMediaProjection(resultCode, data);
		m_VirtualDisplay = createVirtualDisplay();
	}

	private void shareScreen()
	{
		m_ScreenSharing = true;
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
		m_ScreenSharing = false;
		if (m_VirtualDisplay == null)
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
