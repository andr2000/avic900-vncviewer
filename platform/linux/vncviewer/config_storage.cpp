#include <string.h>
#include "compat.h"
#include "config_storage.h"

ConfigStorage *ConfigStorage::m_Instance = NULL;
const std::string ConfigStorage::SECTION_NAME = "General";

ConfigStorage::ConfigStorage() {
	m_ArgC = 0;
	m_ArgV = NULL;
	m_Instance = this;
}

ConfigStorage::~ConfigStorage() {
}

void ConfigStorage::Initialize(std::string &exe, std::string &ini) {
	/* if fails, then use default parameters */
	INIReader::Initialize(ini);
	Clear();
	m_Args.push_back(exe);
	/* the last one MUST be the server ip + port */
	m_Args.push_back(GetServer());
	m_ArgC = m_Args.size();
	Prepare();
}

void ConfigStorage::Clear() {
	if (m_ArgV) {
		for (int i = 0; i < m_ArgC; i++) {
			delete[] m_ArgV[i];
		}
		delete[] m_ArgV;
	}
	m_Args.clear();
	m_ArgC = 0;
	m_ArgV = NULL;
}

void ConfigStorage::Prepare() {
	m_ArgV = new char *[m_Args.size()];
	for (size_t i = 0; i < m_Args.size(); i++) {
		m_ArgV[i] = new char[MAX_PATH + 1];
		strcpy(m_ArgV[i], m_Args[i].c_str());
	}
}

std::string ConfigStorage::GetServer() {
	return Get(SECTION_NAME, "Server", "192.168.2.1:5901");
}

bool ConfigStorage::LoggingEnabled() {
#ifdef DEBUG
	bool def = true;
#else
	bool def = false;
#endif
	return GetBoolean(SECTION_NAME, "Logging", def);
}
