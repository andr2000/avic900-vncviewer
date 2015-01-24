package com.a2k.vncserver;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import org.apache.http.conn.util.InetAddressUtils;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.location.LocationManager;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.opengl.GLES20;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.method.ScrollingMovementMethod;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.a2k.vncserver.VncJni;

public class MainActivity extends Activity implements SurfaceTexture.OnFrameAvailableListener,
	VncJni.NotificationListener
{
	private class BackgroundTask extends AsyncTask<Integer, Void, Void>
	{
		@Override
		protected Void doInBackground(Integer... cmd)
		{
			switch (cmd[0])
			{
				case BACKGROUND_TASK_INIT:
				{
					m_VncJni = new VncJni();
					m_VncJni.setNotificationListener(MainActivity.this);
					m_VncJni.init(getFilesDir().getParent());
					Log.d(TAG, m_VncJni.protoGetVersion());
					m_Rooted = Shell.isSuAvailable();
					setRotation();
					runOnUiThread(new Runnable()
					{
						@Override
						public void run()
						{
							if (m_LogView != null)
							{
								if (m_Rooted)
								{
									m_LogView.append("Rooted device\n");
								}
								else
								{
									m_LogView.append("This device is NOT rooted\n");
								}
							}
							m_ButtonStartStop.setEnabled(true);
						}
					});
					break;
				}
				case BACKGROUND_TASK_START_SERVER:
				{
					runOnUiThread(new Runnable()
					{
						@Override
						public void run()
						{
							if (m_LogView != null)
							{
								readPreferences();
							}
						}
					});
					startup();
					break;
				}
				case BACKGROUND_TASK_STOP_SERVER:
				{
					cleanup();
					break;
				}
				default:
				{
					break;
				}
			}
			return null;
		}
	}

	public static final String TAG = "vncserver";
	private static final int PERMISSION_CODE = 1;
	private static final String MESSAGE_KEY = "text";

	private static final int BACKGROUND_TASK_INIT = 0;
	private static final int BACKGROUND_TASK_START_SERVER = 1;
	private static final int BACKGROUND_TASK_STOP_SERVER = 2;

	private int m_DisplayWidth = 800;
	private int m_DisplayHeight = 480;
	private int m_PixelFormat = GLES20.GL_RGB565;
	private Surface m_Surface;
	private TextureRender m_TextureRender;
	private SurfaceTexture m_SurfaceTexture;

	private int m_CurrentRotation = -1;

	private MediaProjectionManager m_ProjectionManager;
	private MediaProjection m_MediaProjection;
	private VirtualDisplay m_VirtualDisplay;

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;
	private int m_NumClientsConnected = 0;
	private boolean m_KeepScreenOn = false;
	private boolean m_DisplayOff = false;
	private boolean m_SendFullUpdates = false;
	private boolean m_ActivateHotspot = true;
	private boolean m_ActivateAutoRotate = true;
	private boolean m_ActivateGPS = true;
	private int m_SavedBrightnessValue;
	private static PowerManager.WakeLock m_WakeLock = null;
	private Thread m_HelperThread = null;
	private boolean m_HelperThreadRunning = false;

	private TextView m_LogView;
	
	private boolean m_Rooted = false;

	private VncJni m_VncJni = null;

	private NetworkChangeReceiver m_NetworkChangeReceiver = null;
	private int m_TetheringNumActive = -1;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		m_ProjectionManager = (MediaProjectionManager)getSystemService(Context.MEDIA_PROJECTION_SERVICE);

		m_LogView = (TextView)findViewById(R.id.textViewIP);
		m_LogView.setMovementMethod(new ScrollingMovementMethod());

		m_ButtonStartStop = (Button)findViewById(R.id.buttonStartStop);
		m_ButtonStartStop.setEnabled(false);
		m_ButtonStartStop.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				if (m_ProjectionStarted)
				{
					(new BackgroundTask()).execute(BACKGROUND_TASK_STOP_SERVER);
					m_ButtonStartStop.setText("Start");
				}
				else
				{
					(new BackgroundTask()).execute(BACKGROUND_TASK_START_SERVER);
					m_ButtonStartStop.setText("Stop");
				}
				m_ProjectionStarted ^= true;
			}
		});

		/* do heavy tasks in background, so we don't freeze the UI thread */
		(new BackgroundTask()).execute(BACKGROUND_TASK_INIT);
	}

	@Override
	public void onDestroy()
	{
		/* TODO: this is not always called!!! */
		super.onDestroy();
		cleanup();
	}

	private void cleanup()
	{
		if (m_NetworkChangeReceiver != null)
		{
			unregisterReceiver(m_NetworkChangeReceiver);
			m_NetworkChangeReceiver = null;
		}
		stopScreenSharing();
		releaseScreenOn();
		m_VncJni.stopServer();
		restoreRootPermissions();
		enableUtilities(false);
	}

	private void startup()
	{
		setupRootPermissions();
		enableUtilities(true);
		m_VncJni.startServer(m_Rooted, m_DisplayWidth, m_DisplayHeight, m_PixelFormat, m_SendFullUpdates);
		/* listen for tethering changes */
		IntentFilter intentFilter = new IntentFilter("android.net.conn.TETHER_STATE_CHANGED");
		m_NetworkChangeReceiver = new NetworkChangeReceiver();
		registerReceiver(m_NetworkChangeReceiver, intentFilter);
	}

	class NetworkChangeReceiver extends BroadcastReceiver
	{
		@Override
		public void onReceive(Context context, Intent intent)
		{
			ArrayList<String> activeTetherList = intent.getStringArrayListExtra("activeArray");
			int active = activeTetherList.size();
			if (active != m_TetheringNumActive)
			{
				printIPs();
				m_TetheringNumActive = active;
			}
		}
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
			m_LogView.append(bundle.getString(MESSAGE_KEY) + "\n");
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
					/* as we might stop vnc server before receiving disconnect
					 * notifications, so clean up here, so projection can be started */
					m_NumClientsConnected = 0;
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
						releaseScreenOn();
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
			m_LogView.append("User denied screen sharing permission\n");
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
		/* run helper thread */
		m_HelperThread = new Thread(new Runnable()
		{
			@Override
			public void run()
			{
				m_HelperThreadRunning = true;
				while (m_HelperThreadRunning)
				{
					try
					{
						Thread.sleep(1000);
						if (m_DisplayOff && (getBrightness() != 0))
						{
							m_VncJni.setBrightness(0);
						}
						setRotation();
					}
					catch (InterruptedException e)
					{
					}
				}
			}
		});
		m_HelperThread.start();
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
		if (m_HelperThread != null)
		{
			m_HelperThreadRunning = false;
			try
			{
				m_HelperThread.join();
			}
			catch (InterruptedException e)
			{
			}
			m_HelperThread = null;
		}
	}

	private VirtualDisplay createVirtualDisplay()
	{
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		int screenDensity = metrics.densityDpi;
		VirtualDisplay vd = m_MediaProjection.createVirtualDisplay("vncserver",
			m_DisplayWidth, m_DisplayHeight, screenDensity,
			DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
			m_Surface, null /*Callbacks*/, null /*Handler*/);
		if (vd != null)
		{
			keepScreenOn();
		}
		return vd;
	}

	public void onFrameAvailable(SurfaceTexture surfaceTexture)
	{
		if (m_SurfaceTexture != null)
		{
			m_SurfaceTexture.updateTexImage();
			m_TextureRender.drawFrame();
			m_TextureRender.swapBuffers();
		}
	}

	private void setupRootPermissions()
	{
		if (m_Rooted)
		{
			Shell.runCommand("chmod 666 /dev/uinput");
			Shell.runCommand("supolicy --live \"allow untrusted_app input_device dir search\"");
			Shell.runCommand("supolicy --live \"allow untrusted_app uhid_device chr_file open\"");
			Shell.runCommand("supolicy --live \"allow untrusted_app uhid_device chr_file write\"");
			Shell.runCommand("supolicy --live \"allow untrusted_app uhid_device chr_file ioctl\"");
		}
	}

	private void restoreRootPermissions()
	{
		if (m_Rooted)
		{
			Shell.runCommand("chmod 660 /dev/uinput");
			Shell.runCommand("supolicy --live \"deny untrusted_app input_device dir search\"");
			Shell.runCommand("supolicy --live \"deny untrusted_app uhid_device chr_file open\"");
			Shell.runCommand("supolicy --live \"deny untrusted_app uhid_device chr_file write\"");
			Shell.runCommand("supolicy --live \"deny untrusted_app uhid_device chr_file ioctl\"");
		}
	}

	private int getBrightness()
	{
		int brightness = 0;
		try
		{
			brightness = android.provider.Settings.System.getInt(
				getContentResolver(), android.provider.Settings.System.SCREEN_BRIGHTNESS);
		}
		catch (SettingNotFoundException e)
		{
		}
		return brightness;
	}

	private void keepScreenOn()
	{
		if (m_KeepScreenOn)
		{
			PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
			m_WakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "vncserver");
			m_WakeLock.acquire();
		}
		if (m_DisplayOff)
		{
			if (m_VncJni != null)
			{
				m_VncJni.setBrightness(0);
			}
			m_SavedBrightnessValue = getBrightness();
		}
	}

	private void releaseScreenOn()
	{
		if ((m_WakeLock != null) && m_WakeLock.isHeld())
		{
			m_WakeLock.release();
		}
		if (m_DisplayOff)
		{
			m_VncJni.setBrightness(m_SavedBrightnessValue);
		}
	}

	private void setRotation()
	{
		int rotation = getWindowManager().getDefaultDisplay().getRotation();
		if (rotation != m_CurrentRotation)
		{
			if (rotation == Surface.ROTATION_0)
			{
				m_VncJni.setRotation(VncJni.ROTATION_0);
			}
			else if (rotation == Surface.ROTATION_90)
			{
				m_VncJni.setRotation(VncJni.ROTATION_90);
			}
			else if (rotation == Surface.ROTATION_180)
			{
				m_VncJni.setRotation(VncJni.ROTATION_180);
			}
			else if (rotation == Surface.ROTATION_270)
			{
				m_VncJni.setRotation(VncJni.ROTATION_270);
			}
			m_CurrentRotation = rotation;
		}
	}

	private void setWifiApState(boolean isOn)
	{
		WifiManager wifiManager = (WifiManager)getSystemService(Context.WIFI_SERVICE);

		try
		{
			if (wifiManager.isWifiEnabled())
			{
				wifiManager.setWifiEnabled(false);
			}

			/* reflection to get "SetWifiAPEnabled" method */
			Method method = wifiManager.getClass().getMethod("setWifiApEnabled",
				WifiConfiguration.class, boolean.class);
			method.invoke(wifiManager, null, isOn);
		}
		catch (NoSuchMethodException e)
		{
			e.printStackTrace();
		}
		catch (IllegalArgumentException e)
		{
			e.printStackTrace();
		}
		catch (IllegalAccessException e)
		{
			e.printStackTrace();
		}
		catch (InvocationTargetException e)
		{
			e.printStackTrace();
		}
	}

	private void setAutoOrientationEnabled(boolean enabled)
	{
		Settings.System.putInt(this.getContentResolver(),
			Settings.System.ACCELEROMETER_ROTATION, enabled ? 1 : 0);
	}

	private void setGPSEnabled(boolean enabled)
	{
		LocationManager locationManager = (LocationManager)getSystemService(LOCATION_SERVICE);

		boolean providerEnabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
		if ((enabled && !providerEnabled) || (!enabled && providerEnabled))
		{
			Intent intent = new Intent(
				android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
			startActivity(intent);
		}
	}

	private void enableUtilities(boolean enable)
	{
		if (m_ActivateHotspot)
		{
			setWifiApState(enable);
		}
		if (m_ActivateAutoRotate)
		{
			setAutoOrientationEnabled(enable);
		}
		if (m_ActivateGPS)
		{
			setGPSEnabled(enable);
		}
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
				finish();
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
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		boolean use16bit = prefs.getBoolean("use16bit", true);
		m_PixelFormat = use16bit ? GLES20.GL_RGB565 : GLES20.GL_RGBA;
		m_LogView.append("Using pixelformat: " + (m_PixelFormat == GLES20.GL_RGB565 ?
			"RGB565" : "RGBA") + "\n");
		int displaySize = Integer.parseInt(prefs.getString("displaySize", "0"));
		switch (displaySize)
		{
			case 0:
			{
				/* native */
				Display display = getWindowManager().getDefaultDisplay();
				Point size = new Point();
				display.getRealSize(size);
				m_DisplayWidth = size.x;
				m_DisplayHeight = size.y;
				break;
			}
			case 1:
			{
				/* 800 x 480 */
				m_DisplayWidth = 800;
				m_DisplayHeight = 480;
				break;
			}
			case 2:
			{
				m_DisplayWidth = 1280;
				m_DisplayHeight = 720;
				break;
			}
		}
		m_KeepScreenOn = prefs.getBoolean("keepScreenOn", false);
		m_DisplayOff = prefs.getBoolean("displayOff", false) && m_Rooted;
		m_SendFullUpdates = prefs.getBoolean("sendFullUpdates", false);
		m_ActivateHotspot = prefs.getBoolean("activateHotspot", true);
		m_ActivateAutoRotate = prefs.getBoolean("activateAutoRotate", true);
		m_ActivateGPS = prefs.getBoolean("activateGPS", true);
		m_LogView.append("Using framebuffer: " + m_DisplayWidth + "x" + m_DisplayHeight +"\n");
	}

	private List<String> getIpAddresses()
	{
		List<String> list = new ArrayList<String>();
		list.add("localhost: adb forward tcp:5901 tcp:5901");
		try
		{
			for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();)
			{
				NetworkInterface intf = en.nextElement();
				for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();)
				{
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress())
					{
						String ipv4 = inetAddress.getHostAddress();
						if (!inetAddress.isLoopbackAddress() && InetAddressUtils.isIPv4Address(ipv4))
						{
							list.add(ipv4);
						}
					}
				}
			}
		} catch (SocketException e)
		{
			Log.e(TAG, e.toString());
		}
		return list;
	}

	private void printIPs()
	{
		List<String> ipList = getIpAddresses();
		StringBuilder stringList = new StringBuilder();
		stringList.append("Listening on port 5901:\n");
		for (String s : ipList)
		{
			stringList.append("    connect to " + s + "\n");
		}
		m_LogView.append(stringList.toString());
	}
}
