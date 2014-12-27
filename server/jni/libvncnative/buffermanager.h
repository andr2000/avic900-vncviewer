#ifndef BUFFER_MANAGER_H_
#define BUFFER_MANAGER_H_

#include <memory>
#include <mutex>
#include <vector>

class AndroidGraphicBuffer;

class BufferManager
{
public:
	enum MODE
	{
		TRIPLE,
		WITH_COMPARE
	};
	typedef AndroidGraphicBuffer *AndroidGraphicBufferPtr;

	BufferManager() = default;
	~BufferManager();

	bool allocate(MODE mode, int width, int height, int format);
	void release();

	void getConsumer(AndroidGraphicBufferPtr &consumer, AndroidGraphicBufferPtr &compare);
	AndroidGraphicBuffer *getProducer();

private:
	MODE m_Mode { WITH_COMPARE };
	std::mutex m_Lock;
	std::vector<std::unique_ptr<AndroidGraphicBuffer>> m_Buffers;

	int m_Consumer { 0 };
	int m_Producer { 1 };
	int m_Clean { 2 };
	int m_Compare { 3 };
};

#endif /* BUFFER_MANAGER_H_ */
