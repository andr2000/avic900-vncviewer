#include <rfb/rfbclient.h>
#include "client.h"
#include "config_storage.h"

Client *Client::m_Instance = NULL;

#ifndef __linux__
#include "version.inc"
#endif
const char Client::VERSION[] = "AVIC-F900BT VNC client 0.2 " __DATE__ " (" COMMIT_ID ")";
const char Client::COPYRIGHT[] = "(c) 2014-2015 Andrushchenko, Oleksandr andr2000@gmail.com";

Client::Client()
{
	m_Client = NULL;
	m_Thread = NULL;
	m_Mutex = MutexFactory::GetNewMutex();
	m_NeedsVirtInpHack = false;
	m_ConfigStorage = NULL;
	m_ScalingFactorX = 1.0f;
	m_ScalingFactorY = 1.0f;
	m_NeedScaling = false;
	m_LastRefreshTimeMs = -1L;
	m_ForceRefreshToMs = 0;
	m_WaitForMessageToUs = 0;
	m_IsScreenRotated = false;
	memset(&m_UpdateRect, 0, sizeof(m_UpdateRect));
	m_Instance = this;
}

Client::~Client()
{
	Cleanup();
}

void Client::Cleanup()
{
	if (m_Thread)
	{
		m_Thread->Terminate();
		delete m_Thread;
		m_Thread = NULL;
	}
	if (m_Client)
	{
		rfbClientCleanup(m_Client);
		m_Client = NULL;
	}
	m_MessageQueue.clear();
	if (m_Mutex)
	{
		delete m_Mutex;
		m_Mutex = NULL;
	}
	if (m_ConfigStorage)
	{
		delete m_ConfigStorage;
		m_ConfigStorage = NULL;
	}
}

int Client::PollRFB(void *data)
{
	Client *context = reinterpret_cast<Client *>(data);
	return context->Poll();
}

int Client::Initialize()
{
	m_ConfigStorage = ConfigStorage::GetInstance();
	/* set logging options */
	rfbEnableClientLogging = m_ConfigStorage->LoggingEnabled();
	SetLogging();
	/* do we need to hack android's virtual input event? */
	m_NeedsVirtInpHack = m_ConfigStorage->NeedsVirtualInputHack();
	/* force screen refresh? */
	m_ForceRefreshToMs = m_ConfigStorage->ForceRefreshToMs();
	/* is the screen rotated? If so handle Arrows differently */
	m_IsScreenRotated = m_ConfigStorage->IsScreenRotated();
	/* get wait fot message timeout (select), us */
	m_WaitForMessageToUs = m_ConfigStorage->WaitForMessageToUs();;
	rfbClientLog("Initializing VNC Client\n");
	/* get new RFB client */
	m_Client = rfbGetClient(5, 3, 2);
	if (NULL == m_Client)
	{
		return -1;
	}
	/* initialize it */
	m_Client->MallocFrameBuffer = MallocFrameBuffer;
	m_Client->canHandleNewFBSize = FALSE;
	m_Client->GotFrameBufferUpdate = GotFrameBufferUpdate;
	m_Client->FinishedFrameBufferUpdate = FinishedFrameBufferUpdate;
	m_Client->listenPort = -1;
	m_Client->listen6Port = -1;

	/* TODO: for some reason even if I set RGB565 (converted
	 * with reverse shift order to BGR565) it doesn't work.
	 * So, it seems to be 15-bit, but how does it
	 * work on win bgr565 then? */
	m_Client->format.redShift = 10;
	m_Client->format.greenShift = 5;
	m_Client->format.blueShift = 0;
	return 0;
}

std::string Client::GetServerIP()
{
	std::string server = m_ConfigStorage->GetServer();
	return server.erase(server.find_first_of(':'));
}

int Client::Connect()
{
	int argc;
	char **argv;
	argc = m_ConfigStorage->GetArgC();
	argv = m_ConfigStorage->GetArgV();
	if (!rfbInitClient(m_Client, &argc, argv))
	{
		/* rfbInitClient has already freed the client struct */
		m_Client = NULL;
		return -1;
	}
	/* start worker thread */
	if (NULL == m_Thread)
	{
		/* m_Thread will be non-null if we try to reconnect  */
		m_Thread = ThreadFactory::GetNewThread();
		m_Thread->SetWorker(PollRFB, static_cast<void *>(this));
	}
	return m_Thread->Start();
}

void Client::SetupScaling(int width, int height)
{
	m_ScalingFactorX = static_cast<float>(m_Client->width) / width;
	m_ScalingFactorY = static_cast<float>(m_Client->height) / height;
	m_NeedScaling = m_Client->width != width || m_Client->height != height;
}

rfbBool Client::MallocFrameBuffer(rfbClient *client)
{
	return Client::GetInstance()->OnMallocFrameBuffer(client);
}

void Client::GotFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h)
{
	Client::GetInstance()->OnFrameBufferUpdate(client, x, y, w, h);
}

void Client::FinishedFrameBufferUpdate(rfbClient *client)
{
	Client::GetInstance()->OnFinishedFrameBufferUpdate(client);
}

int Client::PostEvent(event_t &evt)
{
	m_Mutex->lock();
	m_MessageQueue.push_back(evt);
	m_Mutex->unlock();
	return 0;
}

int Client::GetEvent(event_t &evt)
{
	int result = 0;

	m_Mutex->lock();
	if (m_MessageQueue.size())
	{
		evt = m_MessageQueue.front();
		m_MessageQueue.pop_front();
		result = 1;
	}
	m_Mutex->unlock();
	return result;
}

void Client::HandleKey(key_t key)
{
	uint32_t rfb_key;

	/* Unfortunately, when screen rotates key mappings for Up/Down/Left/Right
	 * are not handled accordingly, e.g. in Landscape you press Left to go Up etc.
	 * We cannot sense if screen is rotated by means of OnFrameBufferAllocate,
	 * e.g. it always returns the same width and height. So, use a configuration key for that
	 */
	if (m_IsScreenRotated)
	{
		/* remap keys */
		switch (key)
		{
			case KEY_UP:
			{
				key = KEY_RIGHT;
				break;
			}
			case KEY_DOWN:
			{
				key = KEY_LEFT;
				break;
			}
			case KEY_LEFT:
			{
				key = KEY_UP;
				break;
			}
			case KEY_RIGHT:
			{
				key = KEY_DOWN;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	switch (key)
	{
		case KEY_BACK:
		{
			rfb_key = XK_Escape;
			rfbClientLog("Key event: KEY_BACK\n");
			break;
		}
		case KEY_HOME:
		{
			rfb_key = XK_Home;
			rfbClientLog("Key event: KEY_HOME\n");
			break;
		}
		case KEY_UP:
		{
			rfb_key = XK_Up;
			rfbClientLog("Key event: KEY_UP\n");
			break;
		}
		case KEY_DOWN:
		{
			rfb_key = XK_Down;
			rfbClientLog("Key event: KEY_DOWN\n");
			break;
		}
		case KEY_LEFT:
		{
			rfb_key = XK_Left;
			rfbClientLog("Key event: KEY_LEFT\n");
			break;
		}
		case KEY_RIGHT:
		{
			rfb_key = XK_Right;
			rfbClientLog("Key event: KEY_RIGHT\n");
			break;
		}
		default:
		{
			return;
		}
	}
	if (m_NeedsVirtInpHack)
	{
		SendPointerEvent(m_Client, -1, -1, rfbButton1Mask);
	}
	SendKeyEvent(m_Client, rfb_key, TRUE);
	SendKeyEvent(m_Client, rfb_key, FALSE);
	if (m_NeedsVirtInpHack)
	{
		SendPointerEvent(m_Client, -1, -1, 0);
	}
}

int Client::Poll()
{
	int result, evt_count;
	event_t evt;

	result = WaitForMessage(m_Client, m_WaitForMessageToUs);
	if (result < 0)
	{
		/* terminating due to error */
		OnShutdown();
		return result;
	}
	if (result)
	{
		if (!HandleRFBServerMessage(m_Client))
		{
			/* terminating due to error */
			OnShutdown();
			return -1;
		}
	}
	if (m_ForceRefreshToMs && (GetTimeMs() - m_LastRefreshTimeMs > m_ForceRefreshToMs))
	{
		SendFramebufferUpdateRequest(m_Client, 0, 0, m_Client->width, m_Client->height, false);
		m_LastRefreshTimeMs = GetTimeMs();
		rfbClientLog("\nSendFramebufferUpdateRequest\n\n");
	}
	/* checki if there are input events */
	evt_count = 0;
	while ((evt_count < MAX_EVT_PROCESS_AT_ONCE) && GetEvent(evt))
	{
		/* send to the server */
		switch (evt.what)
		{
			case EVT_MOUSE:
				/* fall through */
			case EVT_MOVE:
			{
				SendPointerEvent(m_Client, static_cast<int>(evt.data.point.x * m_ScalingFactorX),
					static_cast<int>(evt.data.point.y * m_ScalingFactorY), evt.data.point.is_down ? rfbButton1Mask : 0);
				rfbClientLog("Mouse event at %d:%d, is_down %d\n", evt.data.point.x, evt.data.point.y,
					evt.data.point.is_down);
				break;
			}
			case EVT_KEY:
			{
				HandleKey(evt.data.key);
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return result;
}

void Client::OnFrameBufferUpdate(rfbClient *cl, int x, int y, int w, int h)
{
	if (m_UpdateRect.x1 + m_UpdateRect.y1 + m_UpdateRect.x2 + m_UpdateRect.y2 == 0)
	{
		/* new frame */
		m_UpdateRect.x1 = x;
		m_UpdateRect.y1 = y;
		m_UpdateRect.x2 = x + w;
		m_UpdateRect.y2 = y + h;
	}
	else
	{
		if (x < m_UpdateRect.x1)
		{
			m_UpdateRect.x1 = x;
		}
		if (m_UpdateRect.x2 < x + w)
		{
			m_UpdateRect.x2 = x + w;
		}
		if (y < m_UpdateRect.y1)
		{
			m_UpdateRect.y1 = y;
		}
		if (m_UpdateRect.y2 < y + h)
		{
			m_UpdateRect.y2 = y + h;
		}
	}
}

void Client::OnFinishedFrameBufferUpdate(rfbClient *client)
{
	/* frame is done */
	memset(&m_UpdateRect, 0, sizeof(m_UpdateRect));
}
