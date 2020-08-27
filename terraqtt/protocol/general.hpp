#ifndef TERRAQTT_PROTOCOL_GENERAL_HPP_
#define TERRAQTT_PROTOCOL_GENERAL_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace terraqtt {

enum class qos
{
	at_most_once,
	at_least_once,
	exactly_once
};

namespace protocol {

typedef std::uint8_t byte;
typedef std::uint32_t variable_integer_type;

enum class variable_integer : variable_integer_type
{
};

enum class control_packet_type
{
	reserved,
	connect,
	connack,
	publish,
	puback,
	pubrec,
	pubrel,
	pubcomp,
	subscribe,
	suback,
	unsubscribe,
	unsuback,
	pingreq,
	pingresp,
	disconnect,
};

struct read_context
{
	std::size_t available = 0;
	std::uint8_t sequence = 0;
	std::uint32_t sequence_data[2]{};
	variable_integer_type remaining_size = 5;

	void clear() noexcept
	{
		sequence         = 0;
		sequence_data[0] = 0;
		sequence_data[1] = 0;
		remaining_size   = 5;
	}
};

class error : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

class protocol_error : public error
{
public:
	using error::error;
};

class io_error : public error
{
public:
	using error::error;
};

// inline std::size_t size_of(byte_istream& input)
// {
// 	const auto pos = input.tellg();

// 	input.seekg(0, std::ios::end);

// 	const auto end = input.tellg();

// 	input.seekg(pos);

// 	return static_cast<std::size_t>(end - pos);
// }

template<typename Element>
constexpr std::size_t elements_max_size() noexcept
{
	return sizeof(typename std::decay<Element>::type);
}

template<typename Element, typename... Elements>
constexpr typename std::enable_if<(sizeof...(Elements) > 0), std::size_t>::type elements_max_size() noexcept
{
	return elements_max_size<Element>() + elements_max_size<Elements...>();
}

template<typename Left, typename Right>
inline bool protected_add(Left& left, Right right) noexcept
{
	static_assert(std::is_unsigned<Left>::value && std::is_unsigned<Right>::value, "must be unsigned");

	if (left > std::numeric_limits<Left>::max() - right) {
		return false;
	}

	left += right;

	return true;
}

} // namespace protocol
} // namespace mqtt

#endif
