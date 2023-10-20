#pragma once

#include "../general.hpp"

namespace terraqtt::protocol::io {

template<typename... Types>
struct PrependLength {
	std::tuple<Types...> value;
};

struct LengthEnd {};

inline std::byte make_first_byte(ControlPacketType type, std::uint8_t flags)
{
	return static_cast<std::byte>(static_cast<int>(type) << 4 | flags);
}

} // namespace terraqtt::protocol::io
