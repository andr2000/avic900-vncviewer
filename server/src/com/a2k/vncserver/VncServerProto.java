package com.a2k.vncserver;

import android.graphics.Bitmap;

public class VncServerProto
{
	public native String getVersion();
	public native void updateScreen(Bitmap bitmap);

	static
	{
		System.loadLibrary("vncserver");
	}
}
