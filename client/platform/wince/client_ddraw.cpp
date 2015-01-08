#include "client_ddraw.h"

rfbBool Client_DDraw::OnMallocFrameBuffer(rfbClient *client)
{
	if (!Client_WinCE::OnMallocFrameBuffer(client))
	{
		return FALSE;
	}
	hdcImage = CreateCompatibleDC(NULL);
	if (!hdcImage)
	{
		return -1;
	}
	SelectObject(hdcImage, m_hBmp);
	return TRUE;
}

void Client_DDraw::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

	HRESULT ddrval;
	HDC hdc;
	if ((ddrval = lpBackBuffer->GetDC(&hdc)) == DD_OK)
	{
		if (!BitBlt(hdc, x, y, w, h, hdcImage, x, y, SRCCOPY))
		{
			ddrval = E_FAIL;
		}
		lpBackBuffer->ReleaseDC(hdc);
	}
	ddrval = lpFrontBuffer->Flip(NULL, 0);
}

void Client_DDraw::OnShutdown()
{
	DeleteDC(hdcImage);
	Client_WinCE::OnShutdown();
}

int Client_DDraw::Initialize(void *_private)
{
	lpDD = NULL;
	lpFrontBuffer = NULL;
	lpBackBuffer = NULL;

	HRESULT ddrval;
	LPDIRECTDRAW pDD;
	ddrval = DirectDrawCreate(NULL, &pDD, NULL);
	if (ddrval != DD_OK)
	{
		return -1;
	}
	/* Fetch DirectDraw4 interface */
	ddrval = pDD->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpDD);
	if (ddrval != DD_OK)
	{
		return -1;
	}
	pDD->Release();
	pDD = NULL;

	ddrval = lpDD->SetCooperativeLevel(m_hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	if (ddrval != DD_OK)
	{
		return -1;
	}
	/* Create surface */
	DDSURFACEDESC2 ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof( ddsd );
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
		DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	ddrval = lpDD->CreateSurface(&ddsd, &lpFrontBuffer, NULL);
	if (ddrval != DD_OK)
	{
		if (ddrval == DDERR_NOFLIPHW)
		{
			return -1;
		}
		return -1;
	}
	/* get a pointer to the back buffer */
	DDSCAPS2 ddscaps;
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	ddrval = lpFrontBuffer->GetAttachedSurface(&ddscaps, &lpBackBuffer);
	if (ddrval != DD_OK)
	{
		return -1;
	}
	return Client_WinCE::Initialize(_private);
}
