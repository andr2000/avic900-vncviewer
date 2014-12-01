package com.a2k.vncserver;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.a2k.vncserver.VncServerProto;

public class MainActivity extends Activity implements TextureView.SurfaceTextureListener
{
	public static final String TAG = "vncserver";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private Surface m_Surface;
	private TextureView m_TextureView;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;

	private static VncServerProto m_VncProto = new VncServerProto();

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Log.d(TAG, m_VncProto.getVersion());

		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		m_ScreenDensity = metrics.densityDpi;

		m_ProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);

		m_TextureView = (TextureView)findViewById(R.id.textureView);
		m_TextureView.setSurfaceTextureListener(this);

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

	@Override
	public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height)
	{
		m_DisplayWidth = width;
		m_DisplayHeight = height;
		m_Surface = new Surface(m_TextureView.getSurfaceTexture());
		m_ButtonStartStop.setEnabled(true);
		m_ButtonStartStop.setText("Start");
	}

	@Override
	public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height)
	{
	}

	@Override
	public boolean onSurfaceTextureDestroyed(SurfaceTexture surface)
	{
		return false;
	}

	@Override
	public void onSurfaceTextureUpdated(SurfaceTexture surface)
	{
		Log.d(TAG, "onSurfaceTextureUpdated");
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
