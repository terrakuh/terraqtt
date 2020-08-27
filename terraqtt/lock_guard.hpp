#ifndef TERRAQTT_LOCK_GUARD_HPP_
#define TERRAQTT_LOCK_GUARD_HPP_

namespace terraqtt {

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

} // namespace terraqtt

#endif
