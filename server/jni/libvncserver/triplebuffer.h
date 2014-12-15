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

	T getConsumer()
	{
		std::lock_guard<std::mutex> lock(m_Lock);
		if (!m_NewBufferAvailable)
		{
			return m_Buffer[m_Consumer];
		}
		m_NewBufferAvailable = false;
		int tmp;
		tmp= m_Clean;
		m_Clean = m_Consumer;
		m_Consumer = tmp;
		return m_Buffer[m_Consumer];
	}

	T getProducer()
	{
		std::lock_guard<std::mutex> lock(m_Lock);
		int tmp;
		tmp= m_Clean;
		m_Clean = m_Producer;
		m_Producer = tmp;
		m_NewBufferAvailable = true;
		return m_Buffer[m_Producer];
	}

private:
	std::mutex m_Lock;
	T m_Buffer[3];
	int m_Consumer = 0;
	int m_Producer = 1;
	int m_Clean = 2;
	bool m_NewBufferAvailable = false;
};

#endif /* LIBVNCSERVER_TRUPLEBUFFER_H_ */
