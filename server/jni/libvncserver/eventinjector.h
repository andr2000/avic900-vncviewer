#include <string>

class EventInjector
{
public:
	EventInjector() = default;
	virtual ~EventInjector() = default;

	void scan();

private:
	static constexpr int BITS_PER_LONG { sizeof(unsigned long) * 8 };

	static constexpr int NBits(unsigned long x)
	{
		return (((x) - 1) / BITS_PER_LONG) + 1;
	}

	static constexpr int TestBit(int nr, const unsigned long *addr)
	{
		return ((addr[nr / BITS_PER_LONG]) >> (nr % BITS_PER_LONG)) & 1;
	}

	static const int INVALID_HANDLE { -1 };
	std::string m_KeypadName;
	int m_KeypadFd { INVALID_HANDLE };
	std::string m_TouchName;
	int m_TouchFd { INVALID_HANDLE };

	int m_TouchValueX = -1;
	int m_TouchMinX;
	int m_TouchMaxX;
	int m_TouchValueY = -1;
	int m_TouchMinY;
	int m_TouchMaxY;

	void probe(const std::string &dev);
	int open(const std::string &dev);
	void close(const int fd);

	enum DEV_CHECK_RET_CODE
	{
		FAILED,
		NO_DEVICE,
		FOUND
	};
	DEV_CHECK_RET_CODE isPointerDev(int fd);
};
