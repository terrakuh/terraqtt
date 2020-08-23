#ifndef MQTT_LOCK_GUARD_HPP_
#define MQTT_LOCK_GUARD_HPP_

namespace mqtt {

template<typename Lockable>
class lock_guard
{
public:
	lock_guard(Lockable& lock) noexcept : _lock(lock)
	{
		lock.lock();
	}
	lock_guard(const Lockable& copy) = delete;
	lock_guard(Lockable&& move)      = delete;
	~lock_guard()
	{
		_lock.unlock();
	}

private:
	Lockable& _lock;
};

} // namespace mqtt

#endif
