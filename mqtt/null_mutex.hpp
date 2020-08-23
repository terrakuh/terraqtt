#ifndef MQTT_NULL_MUTEX_HPP_
#define MQTT_NULL_MUTEX_HPP_

namespace mqtt {

class null_mutex
{
public:
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
