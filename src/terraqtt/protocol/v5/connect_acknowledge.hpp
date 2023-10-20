#pragma once

#include "../general.hpp"
#include "../io/reader.hpp"
#include "property_identifier.hpp"

#include <cassert>
#include <optional>

namespace terraqtt::protocol::v5 {

template<WriteableString StringContainer>
struct ConnectAcknowledgeHeader {
	bool session_present;
	ReasonCode reason_code;
	/// If null, the session expiry specified in the connect header counts.
	std::optional<SessionExpiryInterval> session_expiry;
	std::uint16_t receive_maximum = 65'535;
	QualityOfService maximum_qos = QualityOfService::exactly_once;
	bool retain_available = true;
	std::uint32_t maximum_packet_size = std::numeric_limits<std::uint32_t>::max();
	std::optional<StringContainer> assigned_client_identifier;
	std::uint16_t topic_alias_maximum = 0;
};

template<bool Conservative, io::AsyncContext Context, WriteableString StringContainer>
inline typename Context::template return_type<std::size_t>
  read(io::Reader<5, Conservative, Context>& reader, ConnectAcknowledgeHeader<StringContainer>& header)
{
	assert(reader.packet_type() == ControlPacketType::connect_acknowledge);

	std::error_code ec{};
	std::size_t total_read = 0;

	std::uint8_t connect_flags = 0;
	VariableInteger property_length{};

	std::tie(ec, total_read) = co_await reader.read_fields(connect_flags, header.reason_code, property_length);
	if (ec) {
		co_return reader.context().return_value(ec, total_read);
	}
	header.session_present = static_cast<bool>(reader.control_packet_flags() & 1);

	if (reader.remaining_length() != to_underlying(property_length)) {
		// TODO bad packet
	}

	std::size_t read = 0;
	TERRAQTT_DETAIL_READ_PROPERTIES_BEGIN(ec, read, to_underlying(property_length))
	{
		TERRAQTT_DETAIL_PROPERTY_CASE(session_expiry_interval, header.session_expiry.emplace());

		TERRAQTT_DETAIL_PROPERTY_CASE_BEGIN(receive_maximum, header.receive_maximum)
		TERRAQTT_DETAIL_PROPERTY_CHECK_AND_SET(header.receive_maximum == 0, Error::malformed_receive_maximum)
		TERRAQTT_DETAIL_PROPERTY_CASE_END();

		TERRAQTT_DETAIL_PROPERTY_CASE_BEGIN(maximum_qos, header.maximum_qos)
		TERRAQTT_DETAIL_PROPERTY_CHECK_AND_SET(header.maximum_qos != QualityOfService::at_most_once &&
		                                         header.maximum_qos != QualityOfService::at_least_once,
		                                       Error::malformed_maximum_qos)
		TERRAQTT_DETAIL_PROPERTY_CASE_END();

		TERRAQTT_DETAIL_PROPERTY_CASE(retain_available, header.retain_available);

		TERRAQTT_DETAIL_PROPERTY_CASE(maximum_packet_size, header.maximum_packet_size);

		TERRAQTT_DETAIL_PROPERTY_CASE(topic_alias_maximum, header.topic_alias_maximum);
	}
	TERRAQTT_DETAIL_READ_PROPERTY_END();

	co_return reader.context().return_value(ec, total_read + read);
}

} // namespace terraqtt::protocol::v5
