#ifndef TERRAQTT_STRING_VIEW_HPP_
#define TERRAQTT_STRING_VIEW_HPP_

#include <cstddef>
#include <string>

namespace terraqtt {

class string_view
{
public:
	typedef char value_type;

	string_view(const char* string) noexcept
	{
		_string = string;
		_length = std::char_traits<char>::length(string);
	}
	string_view(const char* string, std::size_t length) noexcept
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

} // namespace mqtt

#endif
