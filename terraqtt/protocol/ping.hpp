#ifndef TERRAQTT_PROTOCOL_PING_HPP_
#define TERRAQTT_PROTOCOL_PING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {
namespace protocol {

struct pingreq_header
{};

struct pingresp_header
{};

template<typename Output>
inline void write_packet(Output& output, const pingreq_header& header)
{
	write_elements(output, static_cast<byte>(static_cast<int>(control_packet_type::pingreq) << 4),
	               static_cast<variable_integer>(0));
}

template<typename Input>
inline bool read_packet(Input& input, read_context& context, pingresp_header& header)
{
	if (context.sequence == 0) {
		byte type;

		if (!read_element(input, context, type)) {
			return false;
		} else if (type != static_cast<byte>(static_cast<int>(control_packet_type::pingresp) << 4)) {
			throw protocol_error{ "invalid pingresp flags" };
		}
	}

	if (context.sequence == 1) {
		variable_integer remaining;

		if (!read_element(input, context, remaining)) {
			return false;
		} else if (remaining != static_cast<variable_integer>(0)) {
			throw protocol_error{ "no payload allow in pingresp" };
		}
	}

	return true;
}

} // namespace protocol
} // namespace mqtt

#endif
