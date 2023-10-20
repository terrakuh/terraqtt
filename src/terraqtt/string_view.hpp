#ifndef TERRAQTT_STRING_VIEW_HPP_
#define TERRAQTT_STRING_VIEW_HPP_

#include <cstddef>
#include <string>

namespace terraqtt {

/// A simple string container holding a single `const char*` and its length.
class String_view {
public:
	typedef char value_type;

	/**
	 * Constructor.
	 *
	 * @param string The static memory buffer.
	 * @tparam Size The size of the buffer. The string size equals to `Size - 1`.
	 */
	template<std::size_t Size>
	String_view(const char (&string)[Size]) noexcept
	{
		static_assert(Size > 0, "no string given");
		_string = string;
		_length = Size - 1;
	}
	/**
	 * Constructor.
	 *
	 * @param string A zero terminated string.
	 */
	String_view(const char* string) noexcept
	{
		_string = string;
		_length = std::char_traits<char>::length(string);
	}
	/**
	 * Constructor.
	 *
	 * @param string The string.
	 * @param length The size of the string.
	 */
	String_view(const char* string, std::size_t length) noexcept
	{
		_string = string;
		_length = length;
	}
	String_view(const char* first, const char* last) noexcept
	{
		_string = first;
		_length = last - first;
	}
	const char* begin() const noexcept { return _string; }
	const char* end() const noexcept { return _string + _length; }
	std::size_t size() const noexcept { return _length; }
	String_view substr(std::size_t offset) const noexcept { return { _string + offset, end() }; }

private:
	const char* _string;
	std::size_t _length;
};

} // namespace terraqtt

#endif
