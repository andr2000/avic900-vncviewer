package com.a2k.vncserver;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.content.Intent;

import com.a2k.vncserver.VncServerProto;
import com.a2k.vncserver.VncProjectionActivity;

public class MainActivity extends Activity
{
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Log.d(getPackageCodePath(), m_VncProto.getVersion());

		m_ButtonStartStop = (Button)findViewById(R.id.buttonStartStop);
		m_ButtonStartStop.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View v)
			{
				if (m_ProjectionStarted)
				{
					m_ButtonStartStop.setText("Start");
				}
				else
				{
					Intent intent = new Intent(MainActivity.this, VncProjectionActivity.class);
					startActivity(intent);
					m_ButtonStartStop.setText("Stop");
					finish();
				}
				m_ProjectionStarted ^= true;
			}
		});
	}

	private Button m_ButtonStartStop;
	private boolean m_ProjectionStarted;
	private static VncServerProto m_VncProto = new VncServerProto();
}
