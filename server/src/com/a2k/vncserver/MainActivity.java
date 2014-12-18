package com.a2k.vncserver;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.opengl.GLES20;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.a2k.vncserver.VncJni;

public class MainActivity extends Activity implements SurfaceTexture.OnFrameAvailableListener,
	VncJni.NotificationListener
{
	public static final String TAG = "MainActivity";
	private static final int PERMISSION_CODE = 1;
	private static final String MESSAGE_KEY = "text";

	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private int m_PixelFormat = GLES20.GL_RGB565;
	private Surface m_Surface;
	private TextureRender m_TextureRender;
	private SurfaceTexture m_SurfaceTexture;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;
	private int m_NumClientsConnected = 0;

	private VncJni m_VncJni = new VncJni();
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		m_VncJni.setNotificationListener(this);
		m_VncJni.init();
		Log.d(TAG, m_VncJni.protoGetVersion());

		m_ProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);

		m_ButtonStartStop = (Button)findViewById(R.id.buttonStartStop);
		m_ButtonStartStop.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				if (m_ProjectionStarted)
				{
					m_ButtonStartStop.setText("Start");
					m_VncJni.stopServer();
				}
				else
				{
					m_ButtonStartStop.setText("Stop");
					readPreferences();
					m_VncJni.startServer(m_DisplayWidth, m_DisplayHeight, m_PixelFormat);
				}
				m_ProjectionStarted ^= true;
			}
		});
	}

	@Override
	public void onDestroy()
	{
		/* TODO: this is not always called!!! */
		super.onDestroy();
		m_VncJni.stopServer();
		stopScreenSharing();
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
	}

	public void onNotification(int what, String message)
	{
		Message msg = new Message();
		msg.what = what;
		Bundle data = new Bundle();
		data.putString(MESSAGE_KEY, message);
		msg.setData(data);
		m_Handler.sendMessage(msg);
	}

	private Handler m_Handler = new Handler()
	{
		@Override
		public void handleMessage(Message msg)
		{
			Bundle bundle = msg.getData();
			switch (msg.what)
			{
				case VncJni.SERVER_STARTED:
				{
					Toast.makeText(MainActivity.this, bundle.getString(MESSAGE_KEY),
						Toast.LENGTH_SHORT).show();
					break;
				}
				case VncJni.SERVER_STOPPED:
				{
					Toast.makeText(MainActivity.this, bundle.getString(MESSAGE_KEY),
						Toast.LENGTH_SHORT).show();
					break;
				}
				case VncJni.CLIENT_CONNECTED:
				{
					if (m_NumClientsConnected == 0)
					{
						startScreenSharing();
					}
					m_NumClientsConnected++;
					Toast.makeText(MainActivity.this, bundle.getString(MESSAGE_KEY),
						Toast.LENGTH_SHORT).show();
					break;
				}
				case VncJni.CLIENT_DISCONNECTED:
				{
					m_NumClientsConnected--;
					if (m_NumClientsConnected == 0)
					{
						stopScreenSharing();
					}
					Toast.makeText(MainActivity.this, bundle.getString(MESSAGE_KEY),
						Toast.LENGTH_SHORT).show();
					break;
				}
				default:
				{
					Toast.makeText(MainActivity.this, bundle.getString(MESSAGE_KEY),
						Toast.LENGTH_SHORT).show();
					Log.d(TAG, "what = " + msg.what + " text = " + bundle.getString(MESSAGE_KEY));
					break;
				}
			}
		}
	};

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

	synchronized private void startScreenSharing()
	{
		if (m_Surface == null)
		{
			m_TextureRender = new TextureRender(m_VncJni, m_DisplayWidth, m_DisplayHeight, m_PixelFormat);
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
			if (m_VirtualDisplay == null)
			{
				m_VirtualDisplay = createVirtualDisplay();
			}
		}
	}

	synchronized private void stopScreenSharing()
	{
		if (m_SurfaceTexture != null)
		{
			m_SurfaceTexture.setOnFrameAvailableListener(null);
		}
		if (m_VirtualDisplay != null)
		{
			m_VirtualDisplay.release();
			m_VirtualDisplay = null;
		}
		if (m_MediaProjection != null)
		{
			/* this causes issue filed here:
			 * https://code.google.com/p/android/issues/detail?id=81152 */
			m_MediaProjection.stop();
			m_MediaProjection = null;
		}
		if (m_TextureRender != null)
		{
			m_TextureRender.stop();
			m_TextureRender = null;
		}
		if (m_Surface != null)
		{
			m_Surface.release();
			m_Surface = null;
		}
		if (m_SurfaceTexture != null)
		{
			m_SurfaceTexture.release();
			m_SurfaceTexture = null;
		}
	}

	private VirtualDisplay createVirtualDisplay()
	{
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		int screenDensity = metrics.densityDpi;
		return m_MediaProjection.createVirtualDisplay("vncserver",
			m_DisplayWidth, m_DisplayHeight, screenDensity,
			DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
			m_Surface, null /*Callbacks*/, null /*Handler*/);
	}

	public void onFrameAvailable(SurfaceTexture surfaceTexture)
	{
		m_SurfaceTexture.updateTexImage();
		m_TextureRender.drawFrame(m_SurfaceTexture);
		m_TextureRender.swapBuffers();
	}

	public boolean onCreateOptionsMenu(Menu menu)
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.menu, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId())
		{
			case R.id.itemMenuSettings:
			{
				startActivity(new Intent(this, Preferences.class));
				return true;
			}
			case R.id.itemMenuExit:
			{
				m_VncJni.stopServer();
				stopScreenSharing();
				System.exit(1);
				return true;
			}
			default:
			{
				return super.onOptionsItemSelected(item);
			}
		}
	}

	private void readPreferences()
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(MainActivity.this);
		boolean use16bit = prefs.getBoolean("use16bit", true);
		m_PixelFormat = use16bit ? GLES20.GL_RGB565 : GLES20.GL_RGBA;
		int displaySize = Integer.parseInt(prefs.getString("displaySize", "0"));
		switch (displaySize)
		{
			case 0:
			{
				/* native */
				DisplayMetrics metrics = new DisplayMetrics();
				getWindowManager().getDefaultDisplay().getMetrics(metrics);
				m_DisplayWidth = metrics.widthPixels;
				m_DisplayHeight = metrics.heightPixels;
				break;
			}
			case 1:
			{
				/* 800 x 480 */
				m_DisplayWidth = 800;
				m_DisplayHeight = 480;
				break;
			}
		}
	}
}
