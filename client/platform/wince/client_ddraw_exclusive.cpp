#include "client_ddraw_exclusive.h"

Client_DDraw_Exclusive::Client_DDraw_Exclusive() : Client_DDraw()
{
	lpBackBuffer = NULL;
	m_CooperativeLevel = DDSCL_FULLSCREEN;
}

void Client_DDraw_Exclusive::BlitFrontToBack(LPRECT rect) {
	while (true)
	{
		HRESULT ddrval = lpBackBuffer->Blt(rect,
			lpFrontBuffer, rect, DDCKEY_SRCBLT, NULL);
		if (ddrval == DD_OK)
		{
			break;
		}
		if (ddrval == DDERR_SURFACELOST)
		{
			if (!RestoreSurfaces())
			{
				return;
			}
			if (ddrval != DDERR_WASSTILLDRAWING)
			{
				return;
			}
		}
	}
}

BOOL Client_DDraw_Exclusive::Blit(int x, int y, int w, int h)
{
	/* blit from front to back those pixels which will not be updated by the m_UpdateRect */
	RECT rect;
	if (y > 0)
	{
		/* have rectangle on top */
		rect.left = 0;
		rect.top = 0;
		rect.right = m_WindowRect.right;
		rect.bottom = y;
		BlitFrontToBack(&rect);
	}
	if (y + h < m_WindowRect.bottom)
	{
		/* have rectangle at bottom */
		rect.left = 0;
		rect.top = y + h;
		rect.right = m_WindowRect.right;
		rect.bottom = m_WindowRect.bottom;
		BlitFrontToBack(&rect);
	}
	if (x > 0)
	{
		/* have rectangle on left */
		rect.left = 0;
		rect.top = y;
		rect.right = x;
		rect.bottom = y + h;
		BlitFrontToBack(&rect);
	}
	if (x + w < m_WindowRect.right)
	{
		/* have rectangle on right */
		rect.left = x + w;
		rect.top = y;
		rect.right = m_WindowRect.right;
		rect.bottom = y + h;
		BlitFrontToBack(&rect);
	}
	/* now blit the update we have just received */
	if (!Client_DDraw::Blit(x, y, w, h))
	{
		return FALSE;
	}
	DEBUGMSG(TRUE, (TEXT("Client_DDraw_Exclusive::Blit: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
	/* now flip */
	while (true)
	{
		HRESULT ddrval = lpFrontBuffer->Flip(NULL, 0);
		if (ddrval == DD_OK)
		{
			break;
		}
		if (ddrval == DDERR_SURFACELOST)
		{
			if (!RestoreSurfaces())
			{
				return FALSE;
			}
		}
		if (ddrval != DDERR_WASSTILLDRAWING)
		{
			break;
		}
	}
	return TRUE;
}

int Client_DDraw_Exclusive::SetupClipper()
{
	return 0;
}

int Client_DDraw_Exclusive::CreateSurface()
{
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP;
	ddsd.dwBackBufferCount = 1;
	HRESULT ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
	if (ddrval != DD_OK)
	{
		if (ddrval == DDERR_NOFLIPHW)
		{
			DEBUGMSG(TRUE, (TEXT("Display driver doesn't support flipping surfaces!\r\n")));
			ReleaseResources();
			return -1;
		}
		DEBUGMSG(TRUE, (TEXT("CreateSurface FrontBuffer Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	/* get a pointer to the back buffer */
	DDSCAPS ddscaps;
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
#if 0
	ddrval = lpFrontBuffer->GetAttachedSurface(&ddscaps, &lpBackBuffer);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("GetAttachedSurface Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	lpBlitSurface = lpBackBuffer;
#endif
	return 0;
}

void Client_DDraw_Exclusive::ReleaseResources(void)
{
	if (lpBackBuffer != NULL)
	{
		lpBackBuffer->Release();
		lpBackBuffer = NULL;
	}
	Client_DDraw::ReleaseResources();
}

void Client_DDraw_Exclusive::OnPaint(void)
{
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);
	EndPaint(m_hWnd, &ps);
}
