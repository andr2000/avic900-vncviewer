#ifndef _CONFIG_STORAGE_H_
#define _CONFIG_STORAGE_H_

#include <vector>

#include "INIReader.h"

class ConfigStorage : public INIReader {
public:
	ConfigStorage();
	virtual ~ConfigStorage();

	static ConfigStorage *GetInstance() {
		if (NULL == m_Instance) {
			m_Instance = new ConfigStorage();
			return m_Instance;
		}
		return m_Instance;
	}
	void Initialize(std::string &exe, std::string &ini);

	int GetArgC() {
		return m_ArgC;
	}
	char **GetArgV() {
		return m_ArgV;
	}
	bool LoggingEnabled();
	std::string GetServer();
	bool NeedsVirtualInputHack();
protected:
	static ConfigStorage *m_Instance;
	static const std::string SECTION_NAME;

	std::vector <std::string> m_Args;
	int m_ArgC;
	char **m_ArgV;

	void Prepare();
	void Clear();
};

#endif
