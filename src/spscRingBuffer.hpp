#ifndef SPCS_RING_BUFFER_HPP
#define SPCS_RING_BUFFER_HPP

#include <array>
#include <cstddef>
#include <atomic>

// single-producer single-consumer

template<typename T, std::size_t Capacity>
class spscRingBuffer
{
	static_assert(Capacity > 1, "Capacity must be > 1");
	
public:
	bool push(const T& item)
	{
		const std::size_t head = head_.load(std::memory_order_relaxed);
		const std::size_t next = increment(head);

		if (next == tail_.load(std::memory_order_acquire))
			return false; // full

		buffer_[head] = item;
		head_.store(next, std::memory_order_release);
		return true;
	}

	bool push(T&& item)
	{
		const std::size_t head = head_.load(std::memory_order_relaxed);
		const std::size_t next = increment(head);

		if (next == tail_.load(std::memory_order_acquire))
			return false;

		buffer_[head] = std::move(item);
		head_.store(next, std::memory_order_release);
		return true;
	}

	bool pop(T& item)
	{
		const std::size_t tail = tail_.load(std::memory_order_relaxed);

		if (tail == head_.load(std::memory_order_acquire))
			return false; // empty

		item = std::move(buffer_[tail]);
		tail_.store(increment(tail), std::memory_order_release);
		return true;
	}

	bool empty() const
	{
		return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
	}

	bool full() const
	{
		const std::size_t next = increment(head_.load(std::memory_order_acquire));

		return next == tail_.load(std::memory_order_acquire);
	}

	std::size_t size() const
	{
		const std::size_t head = head_.load(std::memory_order_acquire);
		const std::size_t tail = tail_.load(std::memory_order_acquire);

		if (head >= tail)
			return head - tail;

		return Capacity - tail + head;
	}

	constexpr std::size_t capacity() const
	{
		return Capacity - 1; // one slot kept empty
	}

private:
	static constexpr std::size_t increment(std::size_t index)
	{
		return (index + 1) % Capacity;
	}

	std::array<T, Capacity> buffer_{};
	alignas(64) std::atomic<std::size_t> head_{ 0 }; // producer writes
	alignas(64) std::atomic<std::size_t> tail_{ 0 }; // consumer writes
};

template<typename T, std::size_t Capacity>
bool push_drop_oldest(spscRingBuffer<T, Capacity>& q, T item)
{
	if (q.push(std::move(item)))
		return true;

	T discarded;
	q.pop(discarded); // drop oldest
	return q.push(std::move(item));
}

#endif // !1
