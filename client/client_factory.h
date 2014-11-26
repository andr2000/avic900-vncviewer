#ifndef _CLIENT_FACTORY_H
#define _CLIENT_FACTORY_H

#ifdef WINCE
#include "client_wince.h"
#elif __linux__
#include "client_gtk.h"
#else
#error Unsupported OS
#endif

class ClientFactory {
public:
	static Client *GetInstance() {
#ifdef WINCE
		return Client_WinCE::GetInstance_WinCE();
#elif __linux__
		return Client_Gtk::GetInstance_Gtk();
#else
#error Unsupported OS
#endif
	}
};

#endif
