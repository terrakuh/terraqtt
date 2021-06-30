#ifndef TERRAQTT_NULL_MUTEX_HPP_
#define TERRAQTT_NULL_MUTEX_HPP_

namespace terraqtt {

/// No operation mutex without any logic. Useful when the client runs in a single thread environment.
class Null_mutex
{
public:
	Null_mutex()                       = default;
	Null_mutex(const Null_mutex& copy) = delete;
	Null_mutex(Null_mutex&& move)      = delete;
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
