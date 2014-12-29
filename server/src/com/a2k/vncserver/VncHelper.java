package com.a2k.vncserver;

import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.util.Log;

public class VncHelper
{
	private static final String TAG = MainActivity.TAG;

	private boolean m_Initialized = false;
	private static final String BINARY_BASE_NAME = "vnchelper";
	private static final String SOCKET_BASE_NAME = "vnchelper_sock";
	private static final int CONNECT_TO_MS = 1000;

	private static final int CMD_SHUTDOWN = 0;
	private static final int CMD_SET_BRIGHTNESS = 1;

	private String m_SocketName;
	private LocalSocket m_Socket;

	public int init(Context context)
	{
		String binary = context.getFilesDir().getParent() + "/lib/lib" + BINARY_BASE_NAME + ".so";
		Shell.runCommand("chmod 777 " + binary);
		m_SocketName = context.getFilesDir().getParent() + "/files/" + SOCKET_BASE_NAME;
		Shell.runCommand(binary + " " + m_SocketName + "&");
		m_Socket = new LocalSocket();
		try
		{
			m_Socket.connect(new LocalSocketAddress(m_SocketName), CONNECT_TO_MS);
		}
		catch (IOException e)
		{
			Log.e(TAG, "Failed to connect to the helper");
		}
		m_Initialized = true;
		return 0;
	}

	public void stop()
	{
		sendPacket(CMD_SHUTDOWN, 0);
	}

	public void setBrightness(int level)
	{
		if (m_Initialized)
		{
			if (m_Socket.isConnected())
			{
				sendPacket(CMD_SET_BRIGHTNESS, level);
			}
		}
	}
	
	private void sendPacket(int cmd, int value)
	{
		ByteBuffer b = ByteBuffer.allocate(8);
		b.putInt(cmd);
		b.putInt(value);
		try
		{
			m_Socket.getOutputStream().write(b.array());
		}
		catch (IOException e)
		{
			Log.e(TAG, "Failed to connect to the helper");
		}
	}
}
