#ifndef _CLIENT_FACTORY_H
#define _CLIENT_FACTORY_H

#ifdef WINCE
#ifdef VNC_DDRAW
#include "client_ddraw.h"
#else
#include "client_mfc.h"
#endif
#elif __linux__
#include "client_gtk.h"
#else
#error Unsupported OS
#endif

class ClientFactory {
public:
	static Client *GetInstance() {
#ifdef WINCE
#ifdef VNC_DDRAW
		return Client_DDraw::GetInstance_DDraw();
#else
		return Client_MFC::GetInstance_MFC();
#endif
#elif __linux__
		return Client_Gtk::GetInstance_Gtk();
#else
#error Unsupported OS
#endif
	}
};

#endif
