#ifndef TERRAQTT_DETAIL_CONSTRAINED_STREAMBUF_HPP_
#define TERRAQTT_DETAIL_CONSTRAINED_STREAMBUF_HPP_

#include <streambuf>

namespace terraqtt {
namespace detail {

/**
 * Constrains the available readable size of an existing streambuf to a given size. Once the bytes are read
 * they cannot be returned.
 *
 * @private
 */
class Constrained_streambuf : public std::streambuf {
public:
	/**
	 * Constructor.
	 *
	 * @param[in] parent The parent stream buffer.
	 * @param remaining The amount of allowed bytes to be read.
	 */
	Constrained_streambuf(std::streambuf& parent, std::size_t remaining) noexcept : _parent(parent)
	{
		_remaining = remaining;
	}
	/// How many bytes can be read.
	std::size_t remaining() const noexcept { return _remaining; }
	std::size_t release()
	{
		const std::size_t remaining = _remaining - static_cast<std::size_t>(egptr() - gptr());
		_remaining                  = 0;
		setg(nullptr, nullptr, nullptr);
		return remaining;
	}

protected:
	/**
	 * @todo Improve performance batch reading and not one character only.
	 */
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
	/// The internal buffer.
	char _buffer[1];
};

} // namespace detail
} // namespace terraqtt

#endif
