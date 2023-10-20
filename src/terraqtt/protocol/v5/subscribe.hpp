#pragma once

#include "fixed_header.hpp"

namespace terraqtt::protocol::v5 {

enum class RetainHandling : std::uint8_t {

};

template<typename StringContainer>
struct SubscribeOptions {
	StringContainer topic_filter;
	QualityOfService maximum_quality_of_service;
	bool no_local;
	bool retain_as_published;
	RetainHandling retain_handling;
};

template<typename StringContainer, template<typename> typename Container>
struct SubscribeHeader {
	PacketIdentifier packet_identifier;
	// Must greater than 0.
	VariableInteger subscription_identifier;
	// TODO user property
	Container<SubscribeOptions<StringContainer>> topic_options;
};

template<AsyncContext Context, typename StringContainer, template<typename> typename Container>
inline typename Context::template return_type<std::size_t>
  write(Context& context, const SubscribeHeader<StringContainer, Container>& header)
{
	std::uint32_t payload_size = 0;
	for (const auto& option : header.topic_options) {
		payload_size += 3 + option.topic_filter.size();
	}

	std::error_code ec{};
	std::size_t total_written = 0;

	std::tie(ec, total_written) = co_await write_whole_packet(
	  context, ControlPacketType::subscribe, 0, payload_size, header.packet_identifier,
	  PropertyIdentifier::subscription_identifier, header.subscription_identifier);

	for (const auto& option : header.topic_options) {
		if (ec) {
			break;
		}

		std::size_t written   = 0;
		std::tie(ec, written) = co_await write_packed_fields(
		  context, option.topic_filter,
		  static_cast<std::byte>((static_cast<int>(option.retain_handling) << 4) |
		                         (option.retain_as_published << 3) | (option.no_local << 2) |
		                         static_cast<int>(option.maximum_quality_of_service)));
		total_written += written;
	}

	co_return context.return_value(ec, total_written);
}

} // namespace terraqtt::protocol::v5
