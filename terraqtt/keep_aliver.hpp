#ifndef TERRAQTT_KEEP_ALIVER_HPP_
#define TERRAQTT_KEEP_ALIVER_HPP_

#include <chrono>
#include <cstdint>

namespace terraqtt {

typedef std::chrono::duration<std::uint16_t> seconds;

template<typename Clock>
class keep_aliver
{
public:
	keep_aliver(seconds timeout = seconds{ 0 }) noexcept
	{
		_timeout = timeout;
		reset();
	}
	void reset()
	{
		_next_timeout = Clock::now() + _timeout;
		_next_reset   = Clock::time_point::min();
	}
	void start_reset_timeout()
	{
		_next_timeout = Clock::time_point::max();
		_next_reset   = Clock::now() + std::chrono::seconds{ 5 };
	}
	bool needs_keeping_alive() const noexcept
	{
		const auto now = Clock::now();
		return now >= _next_timeout && _next_reset < now;
	}
	bool reset_timeout() const noexcept
	{
		const auto now = Clock::now();
		return now >= _next_reset && now < _next_timeout;
	}

private:
	typename Clock::time_point _next_timeout;
	typename Clock::time_point _next_reset;
	seconds _timeout;
};

} // namespace terraqtt

#endif
