package com.a2k.vncserver;

public class VncJni
{
	public static final int SERVER_STARTED = 0;
	public static final int SERVER_STOPPED = 1;
	public static final int CLIENT_CONNECTED = 2;
	public static final int CLIENT_DISCONNECTED = 3;

	private NotificationListener m_Listener;

	public static interface NotificationListener
	{
		public void onNotification(int what, String message);
	}

	public void setNotificationListener(NotificationListener listener)
	{
		m_Listener = listener;
	}

	public void onNotification(int what, String message)
	{
		if (m_Listener != null)
		{
			m_Listener.onNotification(what, message);
		}
	}

	public native void init();
	public native String protoGetVersion();

	public native void bindNextGraphicBuffer();
	public native void frameAvailable();

	public native int startServer(boolean root, int width, int height, int pixelFormat);
	public native int stopServer();

	static
	{
		System.loadLibrary("vncserver");
	}
}
