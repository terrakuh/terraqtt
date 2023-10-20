#pragma once

#include "../general.hpp"
#include "../io.hpp"

#include <limits>

namespace terraqtt::protocol::v5 {

template<ReadableString String>
struct ConnectHeader {
	bool clean_start = true;
	// TODO Add username/password.
	// TODO Add will.
	/// Maximum allowed interval in seconds between sending two packets. `0` means no timeout.
	KeepAliveDuration keep_alive{ 0 };
	/// Session expiry in seconds. If `0`, the session expires when the connection is closed. If `max`, the
	/// session does not expire.
	SessionExpiryInterval session_expiry = SessionExpiryInterval::max();
	/// The maximum allowed of concurrent QoS 1 and 2 publications.
	std::uint16_t receive_maximum = std::numeric_limits<std::uint16_t>::max();
	/// The maximum allowed size of a packet that the client is willing to process.
	std::uint32_t maximum_packet_size = std::numeric_limits<std::uint32_t>::max();
	/// The highest value the client is willing to accept as a topic alias. `0` means no aliases.
	std::uint16_t topic_alias_maximum = 0;
	/// Request the server to return response information in the connection acknowledge header.
	bool request_response_information = false;
	bool request_problem_information = true;
	// TODO User property.
	// TODO Authentication method and data.
	/// The name of this node and must be present. The server must allow identifier that are 1 to 23 bytes longs
	/// and contain these characters: '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
	String client_identifier;
};

template<io::AsyncContext Context, ReadableString String>
inline typename Context::template return_type<std::size_t> write(io::Writer<5, Context>& writer,
                                                                 const ConnectHeader<String>& header)
{
	co_return co_await writer.write_packet(
	  ControlPacketType::connect, 0, std::string_view{ "MQTT" }, std::uint8_t{ 5 },
	  static_cast<std::uint8_t>(header.clean_start << 1), header.keep_alive,

	  // Write all properties.
	  io::LengthStart<VariableInteger>{},
	  io::Property{ header.session_expiry != SessionExpiryInterval::zero(),
	                PropertyIdentifier::session_expiry_interval, header.session_expiry },
	  io::Property{ header.receive_maximum != std::numeric_limits<std::uint16_t>::max(),
	                PropertyIdentifier::receive_maximum, header.receive_maximum },
	  io::Property{ maximum_packet_size != std::numeric_limits<std::uint32_t>::max(),
	                PropertyIdentifier::maximum_packet_size, header.maximum_packet_size },
	  io::Property{ header.topic_alias_maximum != 0, PropertyIdentifier::topic_alias_maximum,
	                header.topic_alias_maximum },
	  io::Property{ header.request_response_information, PropertyIdentifier::request_response_information,
	                header.request_response_information },
	  io::Property{ !header.request_problem_information, PropertyIdentifier::request_problem_information,
	                header.request_problem_information },
	  io::LengthEnd{},

	  header.client_identifier);
}

} // namespace terraqtt::protocol::v5
