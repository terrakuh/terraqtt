#ifndef TERRAQTT_KEEP_ALIVER_HPP_
#define TERRAQTT_KEEP_ALIVER_HPP_

#include <chrono>
#include <cstdint>

namespace terraqtt {

/// Describes seconds with the maximum supported type.
typedef std::chrono::duration<std::uint16_t> Seconds;

/**
 * Keeps track of the keep-alive state of client connection. This class is not thread safe.
 *
 * @private
 * @tparam Clock A clock type that statisfies the Clock requirements.
 */
template<typename Clock>
class Keep_aliver
{
public:
	/**
	 * Constructor.
	 *
	 * @param timeout The maximum timeout for the connection. If `0`, no timeout is set.
	 */
	Keep_aliver(Seconds timeout = Seconds{ 0 }) noexcept
	{
		_timeout = timeout;
		reset();
	}
	/// Resets the timer for the next required ping.
	void reset()
	{
		_next_ping    = Clock::now() + _timeout;
		_ping_timeout = typename Clock::time_point{};
	}
	/// Marks the ping as completed.
	void complete()
	{
		_ping_timeout = typename Clock::time_point{};
	}
	/// Starts the timeout for the required ping response from the broker.
	void start_ping_timeout()
	{
		_ping_timeout = Clock::now() + std::chrono::seconds{ 15 };
	}
	/// Checks whether a ping operation is required to keep the connection alive.
	bool needs_ping() const noexcept
	{
		return Clock::now() >= _next_ping;
	}
	/// Checks whether the timeout has expired.
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
