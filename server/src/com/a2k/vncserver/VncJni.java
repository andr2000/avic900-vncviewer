package com.a2k.vncserver;

import android.graphics.Bitmap;

public class VncJni
{
	public native String protoGetVersion();

	public native long glGetGraphicsBuffer(int width, int height);
	public native void glPutGraphicsBuffer(long buffer);
	public native boolean glBindGraphicsBuffer(long buffer);
	public native void glOnFrameAvailable(long buffer);
	public native void glDumpFrame(long buffer, String path);
	static
	{
		System.loadLibrary("vncserver");
	}
}
