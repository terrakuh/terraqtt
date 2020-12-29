#ifndef TERRAQTT_STRING_VIEW_HPP_
#define TERRAQTT_STRING_VIEW_HPP_

#include <cstddef>
#include <string>

namespace terraqtt {

class String_view
{
public:
	typedef char Value_type;

	template<std::size_t Size>
	String_view(const char (&string)[Size]) noexcept
	{
		static_assert(Size > 0, "no string given");
		_string = string;
		_length = Size - 1;
	}
	String_view(const char* string) noexcept
	{
		_string = string;
		_length = std::char_traits<char>::length(string);
	}
	String_view(const char* string, std::size_t length) noexcept
	{
		_string = string;
		_length = length;
	}
	const char* begin() const noexcept
	{
		return _string;
	}
	const char* end() const noexcept
	{
		return _string + _length;
	}
	std::size_t size() const noexcept
	{
		return _length;
	}

private:
	const char* _string;
	std::size_t _length;
};

} // namespace terraqtt

#endif
