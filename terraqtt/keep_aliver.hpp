#ifndef TERRAQTT_KEEP_ALIVER_HPP_
#define TERRAQTT_KEEP_ALIVER_HPP_

#include <chrono>
#include <cstdint>

namespace terraqtt {

typedef std::chrono::duration<std::uint16_t> Seconds;

template<typename Clock>
class Keep_aliver
{
public:
	Keep_aliver(Seconds timeout = Seconds{ 0 }) noexcept
	{
		_timeout = timeout;
		reset();
	}
	void reset()
	{
		_next_ping    = Clock::now() + _timeout;
		_ping_timeout = typename Clock::time_point{};
	}
	void start_ping_timeout()
	{
		_ping_timeout = Clock::now() + std::chrono::seconds{ 15 };
	}
	bool needs_ping() const noexcept
	{
		return _ping_timeout == typename Clock::time_point{} && _next_ping < Clock::now();
	}
	bool timed_out() const noexcept
	{
		return _ping_timeout != typename Clock::time_point{} && _ping_timeout < Clock::now();
	}

private:
	typename Clock::time_point _next_ping;
	typename Clock::time_point _ping_timeout;
	Seconds _timeout;
};

} // namespace terraqtt

#endif
