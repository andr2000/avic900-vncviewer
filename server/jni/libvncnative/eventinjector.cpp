#include <linux/input.h>
#include <fcntl.h>

#include "eventinjector.h"
#include "log.h"
#include "rfb/keysym.h"
#include "uinput.h"

EventInjector::~EventInjector()
{
	cleanup();
}

#ifndef UI_SET_PROPBIT
#define UI_SET_PROPBIT _IOW(UINPUT_IOCTL_BASE, 110, int)
#endif
#ifndef INPUT_PROP_DIRECT
#define INPUT_PROP_DIRECT 0x01
#endif
#ifndef BUS_VIRTUAL
#define BUS_VIRTUAL 0x06
#endif

bool EventInjector::initialize(int width, int height, int rotation)
{
	static const char *DEV_UINPUT = "/dev/uinput";
	static const char *DEV_NAME = "VncServer";
	struct uinput_user_dev uinp;

	m_Fd = open(DEV_UINPUT, O_WRONLY | O_NDELAY);
	if (m_Fd < 0)
	{
		LOGE("Failed to open %s", DEV_UINPUT);
		return -1;
	}

	m_Width = width;
	m_Height = height;
	m_TouchSideLength = m_Width > m_Height ? m_Width : m_Height;
	m_TdivW = (float)m_TouchSideLength / m_Width;
	m_TdivH = (float)m_TouchSideLength / m_Height;
	m_Tdiv2 = m_TouchSideLength >> 1;
	/* touch scaling */
	m_IgnoreRight = (float)m_Height * m_Height / m_Width / 2;
	m_IgnoreLeft = - m_IgnoreRight;
	/* scale to the visible area - what touch thinks should be scaled to what device expects */
	m_ScaleX = (float)m_Width / 2 / m_IgnoreRight;

	m_IgnoreBottom = (float)m_Width * m_Width / m_Height / 2;
	m_IgnoreTop = - m_IgnoreBottom;
	/* scale to the visible area - what touch thinks should be scaled to what device expects */
	m_ScaleY = (float)m_Height / 2 / m_IgnoreBottom;

	/* we only support 0 and 90 for the touch screen, coordinates updated
	 * according to display's rotation */
	m_TouchscreenRotation = m_Width > m_Height ? ROTATION_90 : ROTATION_0;
	/* update active area */
	onRotation(rotation);
	memset(&uinp, 0, sizeof(uinp));
	/* make touch screen area a square and put its center at (0; 0)
	 * this simplifies transformations:
	 * 1. no scaling needed while rotating
	 * 2. rotation around 0,0 is trivial */
	uinp.id.bustype = BUS_VIRTUAL;
	uinp.absmin[ABS_X] = uinp.absmin[ABS_Y] = - (m_Tdiv2 -1);
	uinp.absmax[ABS_X] = uinp.absmax[ABS_Y] = m_Tdiv2;
	strncpy(uinp.name, DEV_NAME, UINPUT_MAX_NAME_SIZE);

	/* TODO: check for errors */
	ioctl(m_Fd, UI_SET_EVBIT, EV_ABS);
	ioctl(m_Fd, UI_SET_EVBIT, EV_SYN);
	ioctl(m_Fd, UI_SET_EVBIT, EV_KEY);

	ioctl(m_Fd, UI_SET_ABSBIT, ABS_X);
	ioctl(m_Fd, UI_SET_ABSBIT, ABS_Y);
	ioctl(m_Fd, UI_SET_KEYBIT, BTN_TOUCH);
	/* this is to make us touchscreen */
	ioctl(m_Fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
	/* FIXME: the supported range should be KEY_MAX */
	for (int i = 0; i < 0xff; i++)
	{
		ioctl(m_Fd, UI_SET_KEYBIT, i);
	}

	write(m_Fd, &uinp, sizeof(uinp));
	if (ioctl(m_Fd, UI_DEV_CREATE))
	{
		LOGE("Failed to create new uinput device");
		close(m_Fd);
		return -1;
	}
	return m_Fd != INVALID_HANDLE;
}

void EventInjector::cleanup()
{
	if (m_Fd != INVALID_HANDLE)
	{
		close(m_Fd);
	}
}

void EventInjector::reportSync()
{
	struct input_event event = {0};

	event.type  = EV_SYN;
	event.code  = SYN_REPORT;
	event.value = 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportKey(int key, bool press)
{
	struct input_event event = {0};

	event.type  = EV_KEY;
	event.code  = key;
	event.value = press ? 1 : 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportAbs(int code, int value)
{
	struct input_event event = {0};

	event.type  = EV_ABS;
	event.code  = code ;
	event.value = value;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::handlePointerEvent(int buttonMask, int inX, int inY, rfbClientPtr cl)
{
	int x, y;

	if (!transformCoordinates(inX, inY, x, y))
	{
		/* ignore this event */
		return;
	}

	if ((buttonMask & 1) && m_LeftClicked)
	{
		/* left button clicked and moving */
		reportAbs(ABS_X, x);
		reportAbs(ABS_Y, y);
		reportSync();
	}
	else if (buttonMask & 1)
	{
		/* left button clicked */
		m_LeftClicked = true;
		reportAbs(ABS_X, x);
		reportAbs(ABS_Y, y);
		reportKey(BTN_TOUCH, true);
		reportSync();
	}
	else if (m_LeftClicked)
	{
		/* left button released */
		m_LeftClicked = false;
		reportKey(BTN_TOUCH, false);
		reportSync();
	}
}

int EventInjector::transformKey(int keysym)
{
	if (keysym == 0)
	{
		return -1;
	}

	switch(keysym)
	{
		case XK_Escape:			return KEY_ESC;
		case XK_1:				return KEY_1;
		case XK_2:				return KEY_2;
		case XK_3:				return KEY_3;
		case XK_4:				return KEY_4;
		case XK_5:				return KEY_5;
		case XK_6:				return KEY_6;
		case XK_7:				return KEY_7;
		case XK_8:				return KEY_8;
		case XK_9:				return KEY_9;
		case XK_0:				return KEY_0;
		case XK_exclam:			return KEY_1;
		case XK_at:				return KEY_2;
		case XK_numbersign:		return KEY_3;
		case XK_dollar:			return KEY_4;
		case XK_percent:		return KEY_5;
		case XK_asciicircum:	return KEY_6;
		case XK_ampersand:		return KEY_7;
		case XK_asterisk:		return KEY_8;
		case XK_parenleft:		return KEY_9;
		case XK_parenright:		return KEY_0;
		case XK_minus:			return KEY_MINUS;
		case XK_underscore:		return KEY_MINUS;
		case XK_equal:			return KEY_EQUAL;
		case XK_plus:			return KEY_EQUAL;
		case XK_BackSpace:		return KEY_BACKSPACE;
		case XK_Tab:			return KEY_TAB;
		case XK_q:				return KEY_Q;
		case XK_Q:				return KEY_Q;
		case XK_w:				return KEY_W;
		case XK_W:				return KEY_W;
		case XK_e:				return KEY_E;
		case XK_E:				return KEY_E;
		case XK_r:				return KEY_R;
		case XK_R:				return KEY_R;
		case XK_t:				return KEY_T;
		case XK_T:				return KEY_T;
		case XK_y:				return KEY_Y;
		case XK_Y:				return KEY_Y;
		case XK_u:				return KEY_U;
		case XK_U:				return KEY_U;
		case XK_i:				return KEY_I;
		case XK_I:				return KEY_I;
		case XK_o:				return KEY_O;
		case XK_O:				return KEY_O;
		case XK_p:				return KEY_P;
		case XK_P:				return KEY_P;
		case XK_braceleft:		return KEY_LEFTBRACE;
		case XK_braceright:		return KEY_RIGHTBRACE;
		case XK_bracketleft:	return KEY_LEFTBRACE;
		case XK_bracketright:	return KEY_RIGHTBRACE;
		case XK_Return:			return KEY_ENTER;
		case XK_Control_L:		return KEY_LEFTCTRL;
		case XK_a:				return KEY_A;
		case XK_A:				return KEY_A;
		case XK_s:				return KEY_S;
		case XK_S:				return KEY_S;
		case XK_d:				return KEY_D;
		case XK_D:				return KEY_D;
		case XK_f:				return KEY_F;
		case XK_F:				return KEY_F;
		case XK_g:				return KEY_G;
		case XK_G:				return KEY_G;
		case XK_h:				return KEY_H;
		case XK_H:				return KEY_H;
		case XK_j:				return KEY_J;
		case XK_J:				return KEY_J;
		case XK_k:				return KEY_K;
		case XK_K:				return KEY_K;
		case XK_l:				return KEY_L;
		case XK_L:				return KEY_L;
		case XK_semicolon:		return KEY_SEMICOLON;
		case XK_colon:			return KEY_SEMICOLON;
		case XK_apostrophe:		return KEY_APOSTROPHE;
		case XK_quotedbl:		return KEY_APOSTROPHE;
		case XK_grave:			return KEY_GRAVE;
		case XK_asciitilde:		return KEY_GRAVE;
		case XK_Shift_L:		return KEY_LEFTSHIFT;
		case XK_backslash:		return KEY_BACKSLASH;
		case XK_bar:			return KEY_BACKSLASH;
		case XK_z:				return KEY_Z;
		case XK_Z:				return KEY_Z;
		case XK_x:				return KEY_X;
		case XK_X:				return KEY_X;
		case XK_c:				return KEY_C;
		case XK_C:				return KEY_C;
		case XK_v:				return KEY_V;
		case XK_V:				return KEY_V;
		case XK_b:				return KEY_B;
		case XK_B:				return KEY_B;
		case XK_n:				return KEY_N;
		case XK_N:				return KEY_N;
		case XK_m:				return KEY_M;
		case XK_M:				return KEY_M;
		case XK_comma:			return KEY_COMMA;
		case XK_less:			return KEY_COMMA;
		case XK_period:			return KEY_DOT;
		case XK_greater:		return KEY_DOT;
		case XK_slash:			return KEY_SLASH;
		case XK_question:		return KEY_SLASH;
		case XK_Shift_R:		return KEY_RIGHTSHIFT;
		case XK_KP_Multiply:	return KEY_KPASTERISK;
		case XK_Alt_L:			return KEY_LEFTALT;
		case XK_space:			return KEY_SPACE;
		case XK_Caps_Lock:		return KEY_CAPSLOCK;
		case XK_F1:				return KEY_F1;
		case XK_F2:				return KEY_F2;
		case XK_F3:				return KEY_F3;
		case XK_F4:				return KEY_F4;
		case XK_F5:				return KEY_F5;
		case XK_F6:				return KEY_F6;
		case XK_F7:				return KEY_F7;
		case XK_F8:				return KEY_F8;
		case XK_F9:				return KEY_F9;
		case XK_F10:			return KEY_F10;
		case XK_Num_Lock:		return KEY_NUMLOCK;
		case XK_Scroll_Lock:	return KEY_SCROLLLOCK;
		case XK_KP_7:			return KEY_KP7;
		case XK_KP_8:			return KEY_KP8;
		case XK_KP_9:			return KEY_KP9;
		case XK_KP_Subtract:	return KEY_KPMINUS;
		case XK_KP_4:			return KEY_KP4;
		case XK_KP_5:			return KEY_KP5;
		case XK_KP_6:			return KEY_KP6;
		case XK_KP_Add:			return KEY_KPPLUS;
		case XK_KP_1:			return KEY_KP1;
		case XK_KP_2:			return KEY_KP2;
		case XK_KP_3:			return KEY_KP3;
		case XK_KP_0:			return KEY_KP0;
		case XK_KP_Decimal:		return KEY_KPDOT;
		case XK_F13:			return KEY_F13;
		case XK_F11:			return KEY_F11;
		case XK_F12:			return KEY_F12;
		case XK_F14:			return KEY_F14;
		case XK_F15:			return KEY_F15;
		case XK_F16:			return KEY_F16;
		case XK_F17:			return KEY_F17;
		case XK_F18:			return KEY_F18;
		case XK_F19:			return KEY_F19;
		case XK_F20:			return KEY_F20;
		case XK_KP_Enter:		return KEY_KPENTER;
		case XK_Control_R:		return KEY_RIGHTCTRL;
		case XK_KP_Divide:		return KEY_KPSLASH;
		case XK_Sys_Req:		return KEY_SYSRQ;
		case XK_Alt_R:			return KEY_RIGHTALT;
		case XK_Linefeed:		return KEY_LINEFEED;
		case XK_Home:			return KEY_HOME;
		case XK_Up:				return KEY_UP;
		case XK_Page_Up:		return KEY_PAGEUP;
		case XK_Left:			return KEY_LEFT;
		case XK_Right:			return KEY_RIGHT;
		case XK_End:			return KEY_END;
		case XK_Down:			return KEY_DOWN;
		case XK_Page_Down:		return KEY_PAGEDOWN;
		case XK_Insert:			return KEY_INSERT;
		case XK_Delete:			return KEY_DELETE;
		case XK_KP_Equal:		return KEY_KPEQUAL;
		case XK_Pause:			return KEY_PAUSE;
		case XK_F21:			return KEY_F21;
		case XK_F22:			return KEY_F22;
		case XK_F23:			return KEY_F23;
		case XK_F24:			return KEY_F24;
		case XK_KP_Separator:	return KEY_KPCOMMA;
		case XK_Meta_L:			return KEY_LEFTMETA;
		case XK_Meta_R:			return KEY_RIGHTMETA;
		case XK_Multi_key:		return KEY_COMPOSE;
		default:				return -1;
	}
}

int EventInjector::transformSpecialKey(int code)
{
	/* map special keys according to Android's Generic.kl */
	switch (code)
	{
		case KEY_HOME:			return KEY_HOMEPAGE;
		default:
		{
			break;
		}
	}
	return code;
}

void EventInjector::handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	int code = transformKey(key);
	if ((code > 0) && down)
	{
		code = transformSpecialKey(code);
		reportKey(code, true);
		reportKey(code, false);
		reportSync();
	}
	else
	{
		/* key release */
	}
}

void EventInjector::onRotation(int rotation)
{
	m_DisplayRotation = rotation;
	LOGD("display rotation is %d", m_DisplayRotation);
}

bool EventInjector::transformCoordinates(int inX, int inY, int &outX, int &outY)
{
	/* rotate around 0,0 */
	switch (m_DisplayRotation)
	{
		case ROTATION_0:
		{
			outX = inX * m_TdivW - m_Tdiv2;
			outY = inY * m_TdivH - m_Tdiv2;
			break;
		}
		case ROTATION_90:
		{
			outY = inX * m_TdivW - m_Tdiv2;
			outX = (m_Height - inY) * m_TdivH - m_Tdiv2;
			break;
		}
		case ROTATION_180:
		{
			outX = (m_Width - inX) * m_TdivW - m_Tdiv2;
			outY = (m_Height - inY) * m_TdivH - m_Tdiv2;
			break;
		}
		case ROTATION_270:
		{
			outX =inY * m_TdivH - m_Tdiv2;
			outY = (m_Width - inX) * m_TdivW - m_Tdiv2;
			break;
		}
		default:
		{
			LOGE("Unknown display rotation angle");
		}
	}
	/* FIXME: assuming that device's normal orientation is portrait
	 * when width < height
	 * if orientation of the framebuffer and device differ, then
	 * some of the configurations may look as picture in picture,
	 * e.g. scaled device's image fits into framebufer, thus making
	 * black dead zones either on the sides or at top and bottom */
	if (m_TouchscreenRotation == ROTATION_0)
	{
		if ((m_DisplayRotation == ROTATION_90) || (m_DisplayRotation == ROTATION_270))
		{
			/* dead zone is at top and bottom */
			if ((outY < m_IgnoreTop) || (outY > m_IgnoreBottom))
			{
				/* ignore */
				return false;
			}
			/* scale to the visible area - what touch thinks should be scaled to what device expects */
			outY *= m_ScaleY;
		}
	}
	else if (m_TouchscreenRotation == ROTATION_90)
	{
		if ((m_DisplayRotation == ROTATION_0) || (m_DisplayRotation == ROTATION_180))
		{
			/* dead zone is on the left and right */
			if ((outX < m_IgnoreLeft) || (outX > m_IgnoreRight))
			{
				/* ignore */
				return false;
			}
			/* scale to the visible area - what touch thinks should be scaled to what device expects */
			outX *= m_ScaleX;
		}
	}
	return true;
}
