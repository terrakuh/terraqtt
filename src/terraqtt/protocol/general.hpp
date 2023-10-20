#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace terraqtt::protocol {

using KeepAliveDuration = std::chrono::duration<std::uint16_t>;
using SessionExpiryInterval = std::chrono::duration<std::uint32_t>;
using PacketIdentifier = std::uint16_t;

enum class QualityOfService : std::uint8_t {
	at_most_once,
	at_least_once,
	exactly_once,
};

enum class VariableInteger : std::uint32_t {};

template<typename Type>
  requires(std::is_enum_v<Type>)
constexpr std::underlying_type_t<Type> to_underlying(Type value) noexcept
{
	return static_cast<std::underlying_type_t<Type>>(value);
}

enum class ControlPacketType : std::uint8_t {
	connect = 1,
	connect_acknowledge = 2,
	publish = 3,
	publish_acknowledge = 4,
	publish_received = 5,
	publish_release = 6,
	publish_complete = 7,
	subscribe = 8,
	subscribe_acknowledge = 9,
	unsubscribe = 10,
	unsubscribe_acknowledge = 11,
	ping_request = 12,
	ping_response = 13,
	disconnect = 14,
	authenticate = 15
};

} // namespace terraqtt::protocol
