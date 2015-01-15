#ifndef _CONFIG_STORAGE_H_
#define _CONFIG_STORAGE_H_

#include <vector>

#include "INIReader.h"

class ConfigStorage : public INIReader
{
public:
	enum DRAWING_METHOD
	{
		GDI,
		DDRAW_WINDOWED,
		DDRAW_EXCLUSIVE
	};
	ConfigStorage();
	virtual ~ConfigStorage();

	static ConfigStorage *GetInstance()
	{
		if (NULL == m_Instance)
		{
			m_Instance = new ConfigStorage();
			return m_Instance;
		}
		return m_Instance;
	}
	void Initialize(std::string &exe, std::string &ini);

	int GetArgC()
	{
		return m_Args.size();
	}
	char **GetArgV()
	{
		return m_ArgV;
	}
	bool LoggingEnabled();
	std::string GetServer();
	bool NeedsVirtualInputHack();
	bool CompressionEnabled();
	long ForceRefreshToMs();
	bool IsScreenRotated();
	long GetDrawingMethod();
	long WaitForMessageToUs();

protected:
	static ConfigStorage *m_Instance;
	static const std::string SECTION_NAME;

	std::vector<std::string> m_Args;
	char *m_ArgV[16];

	void Prepare();
};

#endif
