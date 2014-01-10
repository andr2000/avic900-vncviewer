#ifndef _CLIENT_FACTORY_H
#define _CLIENT_FACTORY_H

#ifdef WINCE
#include "client_wince.h"
#else
#error Unsupported OS
#endif

class ClientFactory {
public:
	static Client *GetInstance() {
#ifdef WINCE
		return Client_WinCE::GetInstance_WinCE();
#else
#error Unsupported OS
#endif
	}
};

#endif
