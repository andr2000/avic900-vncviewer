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
	LONG x, y, w, h;

	x = ps.rcPaint.left;
	y = ps.rcPaint.top;
	w = ps.rcPaint.right - ps.rcPaint.left;
	h = ps.rcPaint.bottom - ps.rcPaint.top;

	DEBUGMSG(TRUE, (TEXT("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
	if (m_NeedScaling)
	{
		/* blit all */
		 StretchBlt(dc, m_WindowRect.left, m_WindowRect.top,
			 m_WindowRect.right - m_WindowRect.left,
			 m_WindowRect.bottom - m_WindowRect.top,
			 hdcImage, 0, 0, m_Client->width, m_Client->height, SRCCOPY);

	}
	else
	{
		BitBlt(dc, x, y, w, h, hdcImage, x, y, SRCCOPY);
	}
#ifdef SHOW_POINTER_TRACE
	{
		HBRUSH brush_red = CreateSolidBrush(RGB(255, 0, 0));
		HBRUSH brush_green = CreateSolidBrush(RGB(0, 255, 0));
		HBRUSH brush_blue = CreateSolidBrush(RGB(0, 0, 255));

		HPEN pen_red = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		HPEN pen_green = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
		HPEN pen_blue = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));

		HGDIOBJ old_brush = SelectObject(dc, brush_red);
		HGDIOBJ old_pen = SelectObject(dc, pen_red);
		HBRUSH brush;
		HPEN pen;
		for (size_t i = 0; i < m_TraceQueue.size(); i++)
		{
			switch (m_TraceQueue[i].type)
			{
				case TRACE_POINT_DOWN:
				{
					brush = brush_red;
					pen = pen_red;
					break;
				}
				case TRACE_POINT_MOVE:
				{
					brush = brush_green;
					pen = pen_green;
					break;
				}
				case TRACE_POINT_UP:
				{
					brush = brush_blue;
					pen = pen_blue;
					break;
				}
				default:
				{
					break;
				}
			}
			RECT r;
			r.left = m_TraceQueue[i].x;
			r.right = r.left + TRACE_POINT_BAR_SZ;
			r.top = m_TraceQueue[i].y;
			r.bottom = r.top + TRACE_POINT_BAR_SZ;

			SelectObject(dc, &brush_red);
			SelectObject(dc, &pen_red);
			FillRect(dc, &r, brush);

			DEBUGMSG(true, (TEXT("touch at x=%d y=%d\r\n"),
					m_TraceQueue[i].x, m_TraceQueue[i].y));
		}
		SelectObject(dc, old_brush);
		SelectObject(dc, old_pen);
	}
#endif
	EndPaint(m_hWnd, &ps);
}
