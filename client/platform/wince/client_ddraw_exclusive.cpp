#include "client_ddraw_exclusive.h"

Client_DDraw_Exclusive::Client_DDraw_Exclusive() : Client_DDraw()
{
	lpBackBuffer = NULL;
#ifdef WINMOB
	m_CooperativeLevel = DDSCL_FULLSCREEN;
	m_SingleBuffer = false;
#else
	m_CooperativeLevel = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
#endif
}

void Client_DDraw_Exclusive::BlitFrontToBack(LPRECT rect) {
	while (true)
	{
#ifdef WINMOB
		/* TODO: for some reason E_INVALIDARG returnded if DDCKEY_SRCBLT passed */
		HRESULT ddrval = lpBackBuffer->Blt(rect,
			lpFrontBuffer, rect, /*DDCKEY_SRCBLT*/0, NULL);
#else
		HRESULT ddrval = lpBackBuffer->BltFast(rect->left, rect->top,
			lpFrontBuffer, rect, DDBLTFAST_NOCOLORKEY);
#endif
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
		}
		if (ddrval != DDERR_WASSTILLDRAWING)
		{
			return;
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
		HRESULT ddrval;
#ifdef WINMOB
		if (m_SingleBuffer)
		{
			SetRect(&rect, x, y, x + w, y + h);
			/* TODO: for some reason E_INVALIDARG returnded if DDCKEY_SRCBLT passed */
			ddrval = lpFrontBuffer->Blt(&rect, lpBackBuffer, &rect, /*DDBLT_KEYSRC*/0, NULL);
		}
		else
#else
		{
			ddrval = lpFrontBuffer->Flip(NULL, 0);
		}
#endif
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

#ifdef WINMOB
HRESULT PASCAL Client_DDraw_Exclusive::EnumFunction(LPDIRECTDRAWSURFACE pSurface,
	LPDDSURFACEDESC lpSurfaceDesc, LPVOID  lpContext)
{
	static bool bCalled = false;
	if (!bCalled)
	{
		*(static_cast<LPDIRECTDRAWSURFACE *>(lpContext)) = pSurface;
		bCalled = true;
		return DDENUMRET_OK;
	}
	else
	{
		DEBUGMSG(TRUE, (TEXT("DDEX4: Enumerated more than surface?")));
		pSurface->Release();
		return DDENUMRET_CANCEL;
	}
}

int Client_DDraw_Exclusive::CreateSurface()
{
	DDCAPS ddCaps;
	DDCAPS ddHelCaps;
	HRESULT ddrval = lpDD->GetCaps(&ddCaps, &ddHelCaps);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("GetCaps Failed!\r\n")));
		ReleaseResources();
		return -1;
	}

	if (!(ddCaps.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) || !(ddCaps.ddsCaps.dwCaps & DDSCAPS_FLIP))
	{
		m_SingleBuffer = true;
		DEBUGMSG(TRUE, (TEXT("Using single buffer, no flipping\r\n")));
	}

	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if (m_SingleBuffer)
	{
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	}
	else
	{
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP;
		ddsd.dwBackBufferCount = 1;
	}
	ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
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
	if (m_SingleBuffer)
	{
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.dwWidth = m_WindowRect.right - m_WindowRect.left;
		ddsd.dwHeight = m_WindowRect.bottom - m_WindowRect.top;
		ddrval = lpDD->CreateSurface(&ddsd, &lpBackBuffer, NULL);
		if (ddrval != DD_OK)
		{
			DEBUGMSG(TRUE, (TEXT("CreateSurface lpBackBuffer Failed!\r\n")));
			ReleaseResources();
			return -1;
		}
	}
    else
    {
		ddrval = lpFrontBuffer->EnumAttachedSurfaces(&lpBackBuffer, EnumFunction);
		if (ddrval != DD_OK)
		{
			DEBUGMSG(TRUE, (TEXT("EnumAttachedSurfaces Failed!\r\n")));
			ReleaseResources();
			return -1;
		}
	}
	lpBlitSurface = lpBackBuffer;
	return 0;
}
#else
int Client_DDraw_Exclusive::CreateSurface()
{
	DDSURFACEDESC2 ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
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
	DDSCAPS2 ddscaps;
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = lpFrontBuffer->GetAttachedSurface(&ddscaps, &lpBackBuffer);
	if (ddrval != DD_OK)
	{
		DEBUGMSG(TRUE, (TEXT("GetAttachedSurface Failed!\r\n")));
		ReleaseResources();
		return -1;
	}
	lpBlitSurface = lpBackBuffer;
	return 0;
}
#endif

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
