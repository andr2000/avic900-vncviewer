#include <string>

class EventInjector
{
public:
	EventInjector() = default;
	virtual ~EventInjector() = default;

	int initialize(int width, int height);

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
	std::string m_TouchName;
	int m_TouchFd { INVALID_HANDLE };
	int m_TouchWidth = 0;
	int m_TouchHeight = 0;

	/* these are w and h of the uinput device we create */
	int m_Width;
	int m_Height;

	void scan();
	int isPointerDev(int fd);
	bool createDevice();
};
