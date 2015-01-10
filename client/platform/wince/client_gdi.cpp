#include "client_gdi.h"

void Client_GDI::OnFinishedFrameBufferUpdate(rfbClient *client)
{
	RECT ps;

	ps.left = m_UpdateRect.x1;
	ps.top = m_UpdateRect.y1;
	ps.right = m_UpdateRect.x2;
	ps.bottom = m_UpdateRect.y2;
	InvalidateRect(m_hWnd, &ps, FALSE);
	Client_WinCE::OnFinishedFrameBufferUpdate(client);
}

void Client_GDI::OnPaint(void)
{
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(m_hWnd, &ps);
	HDC dcMem;
	LONG x, y, w, h;

	x = ps.rcPaint.left;
	y = ps.rcPaint.top;
	w = ps.rcPaint.right - ps.rcPaint.left;
	h = ps.rcPaint.bottom - ps.rcPaint.top;

	DEBUGMSG(TRUE, (TEXT("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

	dcMem = CreateCompatibleDC(dc);
	HGDIOBJ old_bitmap = SelectObject(dcMem, m_hBmp);
	if (m_NeedScaling)
	{
		/* blit all */
		 StretchBlt(dc, m_WindowRect.left, m_WindowRect.top,
			 m_WindowRect.right - m_WindowRect.left,
			 m_WindowRect.bottom - m_WindowRect.top,
			 dcMem, 0, 0, m_Client->width, m_Client->height, SRCCOPY);

	}
	else
	{
		BitBlt(dc, x, y, w, h, dcMem, x, y, SRCCOPY);
	}
	SelectObject(dcMem, old_bitmap);
	DeleteDC(dcMem);
#ifdef SHOW_POINTER_TRACE
	{
		CBrush *brush, *old_brush;
		CPen *pen, *old_pen;
		RECT r;
		CPen pen_red, pen_green, pen_blue;

		CBrush brush_red(RGB(255, 0, 0));
		CBrush brush_green(RGB(0, 255, 0));
		CBrush brush_blue(RGB(0, 0, 255));

		pen_red.CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		pen_green.CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
		pen_blue.CreatePen(PS_SOLID, 3, RGB(0, 0, 255));

		old_brush = pDC->SelectObject(&brush_red);
		old_pen = pDC->SelectObject(&pen_red);
		for (size_t i = 0; i < m_TraceQueue.size(); i++)
		{
			switch (m_TraceQueue[i].type)
			{
				case TRACE_POINT_DOWN:
				{
					brush = &brush_red;
					pen = &pen_red;
					break;
				}
				case TRACE_POINT_MOVE:
				{
					brush = &brush_green;
					pen = &pen_green;
					break;
				}
				case TRACE_POINT_UP:
				{
					brush = &brush_blue;
					pen = &pen_blue;
					break;
				}
				default:
				{
					break;
				}
			}

			r.left = m_TraceQueue[i].x;
			r.right = r.left + TRACE_POINT_BAR_SZ;
			r.top = m_TraceQueue[i].y;
			r.bottom = r.top + TRACE_POINT_BAR_SZ;

			pDC->SelectObject(&brush_red);
			pDC->SelectObject(&pen_red);
			pDC->FillRect(&r, brush);

			DEBUGMSG(true, (_T("touch at x=%d y=%d\r\n"),
					m_TraceQueue[i].x, m_TraceQueue[i].y));
		}
		pDC->SelectObject(old_brush);
		pDC->SelectObject(old_pen);
	}
#endif
	EndPaint(m_hWnd, &ps);
}
