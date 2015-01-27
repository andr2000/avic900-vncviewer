package com.a2k.vncserver;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.util.Log;

public class VncHelper
{
	private static final String TAG = MainActivity.TAG;

	private static final String BINARY_BASE_NAME = "vnchelper";
	private static final String PIPE_BASE_NAME = "vnchelper_pipe";

	private static final int CMD_SHUTDOWN = 0;
	private static final int CMD_SET_BRIGHTNESS = 1;

	private FileOutputStream m_PipeWriter = null;

	private VncJni m_VncJni;

	public int init(Context context, VncJni jni)
	{
		m_VncJni = jni;
		String binary = context.getFilesDir().getParent() + "/lib/lib" + BINARY_BASE_NAME + ".so";
		Shell.runCommand("chmod 777 " + binary);
		String pipeName = context.getFilesDir().getParent() + "/files/" + PIPE_BASE_NAME;
		/* create a pipe */
		if (m_VncJni.mkfifo(pipeName) == -1)
		{
			Log.d(TAG, "Cannot create " + pipeName);
			return -1;
		}
		Shell.runCommand(binary + " " + pipeName + "&");
		try
		{
			m_PipeWriter = new FileOutputStream(new File(pipeName));
		}
		catch (FileNotFoundException e)
		{
			Log.e(TAG, "Cannot open " + pipeName);
			return -1;
		}
		return 0;
	}

	public void stop()
	{
		sendPacket(CMD_SHUTDOWN, 0);
		try
		{
			if (m_PipeWriter != null)
			{
				m_PipeWriter.close();
			}
		}
		catch (IOException e)
		{
		}
		m_PipeWriter = null;
	}

	public void setBrightness(int level)
	{
		sendPacket(CMD_SET_BRIGHTNESS, level);
	}
	
	private void sendPacket(int cmd, int value)
	{
		byte[] buffer = new byte[8];
		buffer[0] = (byte)((cmd >> 0) & 0xff);
		buffer[1] = (byte)((cmd >> 8) & 0xff);
		buffer[2] = (byte)((cmd >> 16) & 0xff);
		buffer[3] = (byte)((cmd >> 24) & 0xff);
		buffer[4] = (byte)((value >> 0) & 0xff);
		buffer[5] = (byte)((value >> 8) & 0xff);
		buffer[6] = (byte)((value >> 16) & 0xff);
		buffer[7] = (byte)((value >> 24) & 0xff);
		try
		{
			if (m_PipeWriter != null)
			{
				m_PipeWriter.write(buffer);
			}
		}
		catch (IOException e)
		{
			Log.e(TAG, "Cannot write to pipe");
		}
	}
}
