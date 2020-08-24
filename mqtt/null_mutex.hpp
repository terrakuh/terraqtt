#ifndef MQTT_NULL_MUTEX_HPP_
#define MQTT_NULL_MUTEX_HPP_

namespace mqtt {

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

} // namespace mqtt

#endif
