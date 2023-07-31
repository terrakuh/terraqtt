#pragma once

#include "fixed_header.hpp"

#include <chrono>

namespace terraqtt::protocol::v5 {

using MessageExpiryInterval = std::chrono::duration<std::uint32_t>;

enum class PayloadFormatIndicator : std::uint8_t {
	unspecified,
	utf8_data,
};

template<typename StringContainer, typename ByteContainer>
struct PublishHeader {
	bool duplicate;
	QualityOfService quality_of_service;
	bool retain;
	StringContainer topic;
	PacketIdentifier packet_identifier              = 0;
	PayloadFormatIndicator payload_format_indicator = PayloadFormatIndicator::unspecified;
	std::optional<MessageExpiryInterval> message_expiry_interval;
	std::uint16_t topic_alias = 0;
	std::optional<StringContainer> response_topic;
	std::optional<ByteContainer> correlation_data;
	// TODO user property
	// TODO subscription identifier
	std::optional<StringContainer> content_type;
};

template<AsyncContext Context, typename StringContainer, typename ByteContainer>
inline typename Context::template return_type<std::size_t>
  write(Context& context, const PublishHeader<StringContainer, ByteContainer>& header,
        std::uint32_t payload_size)
{
	co_return co_await write_whole_packet(
	  context, ControlPacketType::publish,
	  static_cast<std::uint8_t>((header.duplicate << 3) | (static_cast<int>(header.quality_of_service) << 1) |
	                            header.retain),
	  payload_size, header.topic,
	  header.quality_of_service != QualityOfService::at_most_once ? std::optional{ header.packet_identifier }
	                                                              : std::nullopt,

	  PropertyIdentifier::payload_format_indicator, header.payload_format_indicator,

	  header.message_expiry_interval.has_value()
	    ? std::optional{ std::tuple{ PropertyIdentifier::message_expiry_interval,
	                                 header.message_expiry_interval.value() } }
	    : std::nullopt,

	  header.topic_alias == 0
	    ? std::nullopt
	    : std::optional{ std::tuple{ PropertyIdentifier::topic_alias, header.topic_alias } },

	  header.response_topic.has_value() ? std::optional{ std::forward_as_tuple(
	                                        PropertyIdentifier::response_topic, header.response_topic.value()) }
	                                    : std::nullopt,

	  header.correlation_data.has_value()
	    ? std::optional{ std::forward_as_tuple(PropertyIdentifier::correlation_data,
	                                           header.correlation_data.value()) }
	    : std::nullopt,

	  header.content_type.has_value()
	    ? std::optional{ std::forward_as_tuple(PropertyIdentifier::content_type, header.content_type.value()) }
	    : std::nullopt);
}

} // namespace terraqtt::protocol::v5
