#ifndef TERRAQTT_PROTOCOL_PING_HPP_
#define TERRAQTT_PROTOCOL_PING_HPP_

#include "general.hpp"
#include "reader.hpp"
#include "writer.hpp"

namespace terraqtt {
namespace protocol {

struct Pingreq_header
{};

struct Pingresp_header
{};

template<typename Output>
inline void write_packet(Output& output, std::error_code& ec, const Pingreq_header& header)
{
	write_elements(output, ec, static_cast<Byte>(static_cast<int>(Control_packet_type::pingreq) << 4),
	               static_cast<Variable_integer>(0));
}

template<typename Input>
inline bool read_packet(Input& input, std::error_code& ec, Read_context& context, Pingresp_header& header)
{
	if (context.sequence == 0) {
		Byte type;
		if (!read_element(input, ec, context, type) || ec) {
			return false;
		} else if (type != static_cast<Byte>(static_cast<int>(Control_packet_type::pingresp) << 4)) {
			ec = Error::bad_pingresp_flags;
			return false;
		}
	}

	if (context.sequence == 1) {
		Variable_integer remaining;
		if (!read_element(input, ec, context, remaining) || ec) {
			return false;
		} else if (remaining != static_cast<Variable_integer>(0)) {
			ec = Error::no_pingresp_payload_allowed;
			return false;
		}
	}

	return true;
}

} // namespace protocol
} // namespace terraqtt

#endif
