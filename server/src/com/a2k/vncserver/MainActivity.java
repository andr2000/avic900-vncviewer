package com.a2k.vncserver;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.a2k.vncserver.VncServerProto;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Log.d(getPackageCodePath(), m_VncProto.getVersion());
	}

	private static VncServerProto m_VncProto = new VncServerProto();
}
