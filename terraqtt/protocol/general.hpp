#ifndef TERRAQTT_PROTOCOL_GENERAL_HPP_
#define TERRAQTT_PROTOCOL_GENERAL_HPP_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace terraqtt {

enum class QoS { at_most_once, at_least_once, exactly_once };

namespace protocol {

typedef std::uint8_t Byte;
typedef std::uint32_t Variable_integer_type;

enum class Variable_integer : Variable_integer_type {};

enum class Control_packet_type {
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

struct Read_context {
	/// How much data is available.
	std::size_t available = 0;
	/// The current sequence id.
	std::uint8_t sequence = 0;
	/// Some sequence specific data.
	std::uint32_t sequence_data[2]{};
	/// The remaining size for the current packet.
	Variable_integer_type remaining_size = 5;

	/// Resets the data except for the available field.
	void clear() noexcept
	{
		const auto tmp = available;
		*this          = {};
		available      = tmp;
	}
};

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
} // namespace terraqtt

#endif
