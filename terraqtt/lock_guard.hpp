#ifndef TERRAQTT_LOCK_GUARD_HPP_
#define TERRAQTT_LOCK_GUARD_HPP_

namespace terraqtt {

/**
 * A simple lock guard for systems without the threading library.
 * 
 * @private
 * @tparam Mutex Must statisfy the BasicLockable requirements.
*/
template<typename Lockable>
class Lock_guard
{
public:
	Lock_guard(Lockable& lock) : _lock(lock)
	{
		lock.lock();
	}
	Lock_guard(const Lockable& copy) = delete;
	Lock_guard(Lockable&& move)      = delete;
	~Lock_guard()
	{
		_lock.unlock();
	}

private:
	Lockable& _lock;
};

} // namespace terraqtt

#endif
