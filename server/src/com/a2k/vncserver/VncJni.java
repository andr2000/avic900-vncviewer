package com.a2k.vncserver;

import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class VncJni
{
	public static final int SERVER_STARTED = 0;
	public static final int CLIENT_CONNECTED = 1;
	public static final int CLIENT_DISCONNECTED = 2;

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

	public native long glGetGraphicsBuffer(int width, int height, int pixelFormat);
	public native void glPutGraphicsBuffer(long buffer);
	public native boolean glBindGraphicsBuffer(long buffer);
	public native void glOnFrameAvailable(long buffer);
	public native void glDumpFrame(long buffer, String path);

	public native int startServer(int width, int height, int pixelFormat);

	static
	{
		System.loadLibrary("vncserver");
	}
}
