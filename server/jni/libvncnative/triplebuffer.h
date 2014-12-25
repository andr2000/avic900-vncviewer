#ifndef LIBVNCSERVER_TRUPLEBUFFER_H_
#define LIBVNCSERVER_TRUPLEBUFFER_H_

template <typename T>
class TripleBuffer
{
public:
	TripleBuffer()
	{
	}

	void add(int pos, const T& value)
	{
		m_Buffer[pos] = value;
	}

	void release()
	{
		for (int i = 0; i < NUM_BUFFERS; i++)
		{
			m_Buffer[i] = nullptr;
		}
	}

	void getConsumer(T &consumer, T &compare)
	{
		std::lock_guard<std::mutex> lock(m_Lock);
		m_Compare = m_Consumer;
		int tmp;
		tmp = m_Clean;
		m_Clean = m_Consumer;
		m_Consumer = tmp;
		consumer = m_Buffer[m_Consumer];
		compare = m_Buffer[m_Compare];
	}

	T getProducer()
	{
		std::lock_guard<std::mutex> lock(m_Lock);
		int tmp;
		tmp = m_Clean;
		m_Clean = m_Producer;
		m_Producer = tmp;
		return m_Buffer[m_Producer];
	}

private:
	static const int NUM_BUFFERS = 4;

	std::mutex m_Lock;
	T m_Buffer[NUM_BUFFERS];
	int m_Consumer = 0;
	int m_Producer = 1;
	int m_Clean = 2;
	int m_Compare = 3;
};

#endif /* LIBVNCSERVER_TRUPLEBUFFER_H_ */
