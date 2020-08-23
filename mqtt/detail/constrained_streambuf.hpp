#ifndef MQTT_DETAIL_CONSTRAINED_STREAMBUF_HPP_
#define MQTT_DETAIL_CONSTRAINED_STREAMBUF_HPP_

#include <streambuf>

namespace mqtt {
namespace detail {

class constrained_streambuf : public std::streambuf
{
public:
	constrained_streambuf(std::streambuf& parent, std::size_t remaining) noexcept : _parent(parent)
	{
		_remaining = remaining;
	}
	std::size_t remaining() const noexcept
	{
		return _remaining;
	}

protected:
	int_type underflow() override
	{
		if (gptr() == egptr()) {
			if (!_remaining) {
				return traits_type::eof();
			}

			_parent.sgetn(_buffer, 1);
			setg(_buffer, _buffer, _buffer + 1);

			--_remaining;
		}

		return traits_type::to_int_type(_buffer[0]);
	}

private:
	std::streambuf& _parent;
	std::size_t _remaining;
	char _buffer[1];
};

} // namespace detail
} // namespace mqtt

#endif
