#include <algorithm>
#include <fstream>
#include <memory>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

std::string &leftTrim(std::string &s)
{
	s.erase(s.begin(), find_if_not(s.begin(), s.end(), [](int c){ return isspace(c); }));
	return s;
}

std::string &rightTrim(std::string &s)
{
	s.erase(find_if_not(s.rbegin(), s.rend(), [](int c){ return isspace(c); }).base(), s.end());
	return s;
}

std::string trim(const std::string &s)
{
	std::string tmp = s;
	return leftTrim(rightTrim(tmp));
}

int readValue(std::ifstream &file, const std::string &parameter, std::string &value)
{
	bool found = false;
	size_t pos;
	std::string line;
	while (std::getline(file, line))
	{
		line = trim(line);
		std::transform(line.begin(), line.end(), line.begin(), ::tolower);
		pos = line.find(parameter);
		if (pos == std::string::npos)
		{
			continue;
		}
		found = true;
		break;
	}
	if (!found)
	{
		return -1;
	}
	value = line;
	if (line.find("\\") != std::string::npos)
	{
		/* this is a multi line paramter */
		while ((pos = value.find("\\")) != std::string::npos)
		{
			value.erase(pos, 1);
			if (!std::getline(file, line))
			{
				break;
			}
			std::transform(line.begin(), line.end(), line.begin(), ::tolower);
			line = trim(line);
			value += line;
		}
	}
	return 0;
}

int parseInputFile(const char *fName, std::string &paramStatic)
{
	const std::string REG_KEY { "[hkey_local_machine\\software\\microsoft\\wzcsvc\\parameters\\interfaces\\" };
	const std::string REG_PARAM_STATIC { "\"static#" };
	std::ifstream file;

	file.open(fName, std::ifstream::in);
	if (!file.is_open())
	{
		return -1;
	}
	std::string line;
	bool found = false;
	/* find the registry key */
	while (std::getline(file, line))
	{
		line = trim(line);
		std::transform(line.begin(), line.end(), line.begin(), ::tolower);
		size_t pos = line.find(REG_KEY);
		if (pos == std::string::npos)
		{
			continue;
		}
		found = true;
		break;
	}
	if (!found)
	{
		file.close();
		return -1;
	}
	printf("Found registry entry %s\n", line.c_str());
	/* read connection paramters */
	if (readValue(file, REG_PARAM_STATIC, paramStatic) < 0)
	{
		printf("%s not found\n", REG_PARAM_STATIC.c_str());
	}
	file.close();
	return 0;
}

int convertHex(const std::string &hex, unsigned char &val)
{
	char *p;
	long n = strtol(hex.c_str(), &p, 16);
	if (*p != 0)
	{
		return -1;
	}
	val = static_cast<unsigned char>(n);
	return 0;
}

int parseStatic(std::string &paramStatic, std::vector<unsigned char> &staticValues)
{
	size_t pos = paramStatic.find(":");
	if (pos == std::string::npos)
	{
		return -1;
	}
	paramStatic.erase(0, pos + 1);
	while ((pos = paramStatic.find(",")) != std::string::npos)
	{
		std::string tmp = "0x" + paramStatic.substr(0, pos);
		paramStatic.erase(0, pos + 1);
		unsigned char val;
		if (convertHex(tmp, val) < 0)
		{
			return -1;
		}
		staticValues.push_back(val);
	}
	if (paramStatic.length())
	{
		unsigned char val;
		if (convertHex(paramStatic, val) < 0)
		{
			return -1;
		}
		staticValues.push_back(val);
	}
	return 0;
}

const int BSSID_OFS = 0x08;
const int SSID_LEN_OFS = 0x10;
const int SSID_OFS = 0x14;
const int ENC_TYPE_OFS = 0x34;
const int AUTH_TYPE_OFS = 0x94;

const int ENC_TYPE_WEP = 0x00;
const int ENC_TYPE_DISABLED = 0x01;
const int ENC_TYPE_TKIP = 0x04;
const int ENC_TYPE_AES = 0x06;

const int AUTH_WPA_PSK = 0x04;
const int AUTH_WPA = 0x03;
const int AUTH_SHARED = 0x01;
const int AUTH_OPEN = 0x00;

const char *AuthenticationMode[] =
{
	"Open",
	"Shared",
	"AutoSwitch",
	"WPA",
	"WPAPSK",
	"WPANone",
	"WPA2",
	"WPA2PSK"
};

const char *WZCTOOL_REG =
	"REGEDIT4\r\n" \
	"\r\n" \
	"[HKEY_CURRENT_USER\\Comm\\WZCTOOL]\r\n" \
	"\"SSID\" = \"%s\"\r\n" \
	"\"authentication\" = dword:%d\r\n" \
	"\"encryption\" = dword:%d\r\n" \
	"\"key\" = \"%s\"\r\n" \
	"\"eap\" = \"tls\"\r\n" \
	"\"adhoc\" = dword:0\r\n";

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("Usage: %s <input registry file> <password>\n", argv[0]);
		return 0;
	}
	std::string paramStatic;
	if (parseInputFile(argv[1], paramStatic) < 0)
	{
		printf("Can't find registry entries for wifi\n");
		return 0;
	}
	std::vector<unsigned char> staticValues;
	if (parseStatic(paramStatic, staticValues) < 0)
	{
		printf("Can't parse Static#\n");
		return 0;
	}
	/* read MAC */
	unsigned char mac[6];
	memcpy(mac, &staticValues.data()[BSSID_OFS], sizeof(mac));
	printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	char ssid[32];
	memset(ssid, 0, sizeof(ssid));
	memcpy(ssid, &staticValues.data()[SSID_OFS], staticValues.data()[SSID_LEN_OFS]);
	printf("SSID: %s\n", ssid);
	int encryptionType = staticValues.data()[ENC_TYPE_OFS];
	switch (encryptionType)
	{
		case ENC_TYPE_WEP:
		{
			printf("Encryption: %s\n", "WEP");
			break;
		}
		case ENC_TYPE_DISABLED:
		{
			printf("Encryption: %s\n", "Disabled");
			break;
		}
		case ENC_TYPE_TKIP:
		{
			printf("Encryption: %s\n", "TKIP");
			break;
		}
		case ENC_TYPE_AES:
		{
			printf("Encryption: %s\n", "AES");
			break;
		}
	}
	int authType = staticValues.data()[AUTH_TYPE_OFS];
	printf("Authentication: %s\n", AuthenticationMode[authType]);
	printf(WZCTOOL_REG, ssid, authType, encryptionType, argv[2]);
	return 0;
}
