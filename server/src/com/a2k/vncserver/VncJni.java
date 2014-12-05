package com.a2k.vncserver;

import android.graphics.Bitmap;

public class VncJni
{
	public native String protoGetVersion();

	public native long glGetGraphicsBuffer(int width, int height);
	public native void glPutGraphicsBuffer(long buffer);
	public native boolean glBindGraphicsBuffer(long buffer);
	public native void glOnFrameAvailable(long buffer);

	static
	{
		System.loadLibrary("vncserver");
	}
}
