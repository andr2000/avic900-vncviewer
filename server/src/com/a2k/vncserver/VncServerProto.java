package com.a2k.vncserver;

public class VncServerProto
{
	public native String getVersion();

	static
	{
		System.loadLibrary("vncserver");
	}
}
