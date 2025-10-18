#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>

template<typename TYPE, size_t SIZE>
class CircularBuffer
{
public:
	size_t available()
	{
		size_t h = this->head;
		size_t t = this->tail;
		if (h >= t)
		{
			return h - t;
		} else {
			return (SIZE - t + h);
		}
	}

	void put(TYPE& value)
	{
		size_t next = (head + 1) % SIZE;
		if (next == tail)
		{
			overflow = true;
			return;
		}

		buffer[next] = value;
		head = next;
	}

	bool read(TYPE* out_result)
	{
		if (tail == head)
		{
			out_result = nullptr;
			return false;
		}

		size_t next = (tail + 1) % SIZE;
		*out_result = buffer[next];
		tail = next;
		return true;
	}

	bool overflowDetected()
	{
		return overflow;
	}

private:
	volatile TYPE buffer[SIZE];
	volatile size_t head = 0;
	volatile size_t tail = 0;
	volatile bool overflow;
};

#endif
