package com.a2k.vncserver;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
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
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.a2k.vncserver.VncJni;

public class MainActivity extends Activity implements SurfaceTexture.OnFrameAvailableListener
{
	public static final String TAG = "MainActivity";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private Surface m_Surface;
	private TextureRender m_TextureRender;
	private SurfaceTexture m_SurfaceTexture;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;

	private VncJni m_VncJni = new VncJni();
	
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

		m_ButtonStartStop = (Button)findViewById(R.id.buttonStartStop);
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
			m_TextureRender = new TextureRender(m_VncJni, m_DisplayWidth, m_DisplayHeight);
			m_TextureRender.setDumpOutputDir("/sdcard/");
			m_TextureRender.start();
			m_SurfaceTexture = new SurfaceTexture(m_TextureRender.getTextureId());
			m_SurfaceTexture.setDefaultBufferSize(m_DisplayWidth, m_DisplayHeight);
			m_SurfaceTexture.setOnFrameAvailableListener(this);
			m_Surface = new Surface(m_SurfaceTexture);
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

	public void onFrameAvailable(SurfaceTexture surfaceTexture)
	{
		Log.d(TAG, "onFrameAvailable");
		m_SurfaceTexture.updateTexImage();
		m_TextureRender.drawFrame(m_SurfaceTexture);
		m_TextureRender.swapBuffers();
	}
}
