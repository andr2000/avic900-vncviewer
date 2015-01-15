#ifndef __linux__
#include <windows.h>
#endif
#include <string.h>
#include "compat.h"
#include "config_storage.h"

ConfigStorage *ConfigStorage::m_Instance = NULL;
const std::string ConfigStorage::SECTION_NAME = "General";

ConfigStorage::ConfigStorage()
{
	m_Instance = this;
}

ConfigStorage::~ConfigStorage()
{
}

void ConfigStorage::Initialize(std::string &exe, std::string &ini)
{
	/* if fails, then use default parameters */
	INIReader::Initialize(ini);
	m_Args.clear();
	m_Args.push_back(exe);
	/* supported encodings */
	m_Args.push_back("-encodings");
	if (CompressionEnabled())
	{
		m_Args.push_back("tight zrle ultra copyrect hextile zlib corre rre raw");
	}
	else
	{
		m_Args.push_back("ultra copyrect hextile corre rre raw");
	}
	/* the last one MUST be the server ip + port */
	m_Args.push_back(GetServer());
	Prepare();
}

void ConfigStorage::Prepare()
{
	for (size_t i = 0; i < m_Args.size(); i++)
	{
		m_ArgV[i] = const_cast<char *>(m_Args[i].c_str());
	}
}

std::string ConfigStorage::GetServer()
{
	return Get(SECTION_NAME, "Server", "192.168.2.1:5901");
}

bool ConfigStorage::NeedsVirtualInputHack()
{
	return GetBoolean(SECTION_NAME, "VirtInputHack", true);
}

bool ConfigStorage::LoggingEnabled()
{
#ifdef DEBUG
	bool def = true;
#else
	bool def = false;
#endif
	return GetBoolean(SECTION_NAME, "Logging", def);
}

bool ConfigStorage::CompressionEnabled()
{
	return GetBoolean(SECTION_NAME, "CompressionEnabled", false);
}

long ConfigStorage::ForceRefreshToMs()
{
	return GetInteger(SECTION_NAME, "ForceRefreshToMs", 2000);
}

bool ConfigStorage::IsScreenRotated()
{
	return GetBoolean(SECTION_NAME, "IsScreenRotated", true);
}

long ConfigStorage::GetDrawingMethod()
{
	return GetInteger(SECTION_NAME, "DrawingMethod", 0);
}

long ConfigStorage::WaitForMessageToUs()
{
	return GetInteger(SECTION_NAME, "WaitForMessageToUs", 5000);
}
