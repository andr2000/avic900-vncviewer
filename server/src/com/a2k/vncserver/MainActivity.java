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
import android.os.Environment;
import android.provider.MediaStore.Video.VideoColumns;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.Surface;
import android.view.TextureView.SurfaceTextureListener;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.Toast;

import com.a2k.vncserver.Gles;
import com.a2k.vncserver.VncJni;

public class MainActivity extends Activity implements SurfaceTexture.OnFrameAvailableListener
{
	public static final String TAG = "MainActivity";
	private static final int PERMISSION_CODE = 1;

	private int m_ScreenDensity;
	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private Surface m_Surface;
	private SurfaceTexture m_SurfaceTexture;
	private long m_GraphicBuffer;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;

	private String m_PngOutputFile;

	private VncJni m_VncJni = new VncJni();
	private Gles m_Gles = new Gles();
	
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

		ContextWrapper c = new ContextWrapper(this);
		m_PngOutputFile = c.getFilesDir().getPath() + "/surface.png";

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
			m_Gles.initGL(m_VncJni, m_DisplayWidth, m_DisplayHeight);
			m_GraphicBuffer = m_VncJni.glGetGraphicsBuffer(m_DisplayWidth, m_DisplayHeight);
			m_VncJni.glBindGraphicsBuffer(m_GraphicBuffer);
			m_SurfaceTexture = new SurfaceTexture(m_Gles.getTexture());
			m_SurfaceTexture.setOnFrameAvailableListener(MainActivity.this);
			m_SurfaceTexture.setDefaultBufferSize(m_DisplayWidth, m_DisplayHeight);
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
		m_Gles.deinitGL();
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
		m_SurfaceTexture.updateTexImage();
		m_Gles.draw(m_SurfaceTexture);
		m_VncJni.glOnFrameAvailable(m_GraphicBuffer);
		Gles.saveFrame(m_PngOutputFile, m_DisplayWidth, m_DisplayHeight);
		m_Gles.swapBufers();
	}
}
