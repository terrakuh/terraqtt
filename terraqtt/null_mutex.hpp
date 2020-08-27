#ifndef TERRAQTT_NULL_MUTEX_HPP_
#define TERRAQTT_NULL_MUTEX_HPP_

namespace terraqtt {

class null_mutex
{
public:
	null_mutex()                       = default;
	null_mutex(const null_mutex& copy) = delete;
	null_mutex(null_mutex&& move)      = delete;
	void lock() noexcept
	{}
	void unlock() noexcept
	{}
	bool try_lock() noexcept
	{
		return true;
	}
};

} // namespace terraqtt

#endif
