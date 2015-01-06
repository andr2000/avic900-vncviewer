#include "client_gdi.h"

void Client_MFC::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	RECT ps;

	ps.left = x;
	ps.top = y;
	ps.right = x + w;
	ps.bottom = y + h;
	InvalidateRect(m_hWnd, &ps, FALSE);
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
}

void Client_MFC::OnPaint(void) {
	PAINTSTRUCT ps;
	HDC *pDC = BeginPaint(m_hWnd, &ps);

	CBitmap *bitmap = CBitmap::FromHandle(m_hBmp);
	if (bitmap) {
		CDC dcMem;
		LONG x, y, w, h;

		x = ps.rcPaint.left;
		y = ps.rcPaint.top;
		w = ps.rcPaint.right - ps.rcPaint.left;
		h = ps.rcPaint.bottom - ps.rcPaint.top;

		DEBUGMSG(true, (_T("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

		dcMem.CreateCompatibleDC(pDC);
		CBitmap *old_bitmap = dcMem.SelectObject(bitmap);
		if (m_NeedScaling) {
			pDC->StretchBlt(m_ClientRect.left, m_ClientRect.top, m_ClientRect.right, m_ClientRect.bottom, &dcMem,
				m_ServerRect.left, m_ServerRect.top, m_ServerRect.right, m_ServerRect.bottom, SRCCOPY);
		} else {
			pDC->BitBlt(x, y, w, h, &dcMem, x, y, SRCCOPY);
		}
		dcMem.SelectObject(old_bitmap);
		dcMem.DeleteDC();
	}
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
		for (size_t i = 0; i < m_TraceQueue.size(); i++) {
			switch (m_TraceQueue[i].type) {
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
