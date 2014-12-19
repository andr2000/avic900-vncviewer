#include <string>

class EventInjector
{
public:
	EventInjector() = default;
	virtual ~EventInjector() = default;

	bool initialize(int width, int height);

private:
	int m_Width = 0;
	int m_Height = 0;
};
