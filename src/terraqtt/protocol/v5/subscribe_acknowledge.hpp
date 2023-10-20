#pragma once

#include "../general.hpp"
#include "../io/reader.hpp"
#include "property_identifier.hpp"

#include <cassert>
#include <optional>

namespace terraqtt::protocol::v5 {

template<WriteableString StringContainer>
struct SubscribeAcknowledgeHeader {
	std::optional<StringContainer> reason_string;
	std::vector<ReasonCode> reason_codes;
};

template<bool Conservative, io::AsyncContext Context, WriteableString StringContainer>
inline typename Context::template return_type<std::size_t>
  read(io::Reader<5, Conservative, Context>& reader, SubscribeAcknowledgeHeader<StringContainer>& header)
{
	assert(reader.packet_type() == ControlPacketType::connect_acknowledge);

	std::error_code ec{};
	std::size_t total_read = 0;

	std::tie(ec, total_read) = co_await reader.read_fields(property_length);
	if (ec) {
		co_return reader.context().return_value(ec, total_read);
	}

	std::size_t read = 0;
	TERRAQTT_DETAIL_READ_PROPERTIES_BEGIN(ec, read, to_underlying(property_length))
	{
		TERRAQTT_DETAIL_PROPERTY_CASE(reason_string, header.reason_string.emplace());
	}
	TERRAQTT_DETAIL_READ_PROPERTY_END();

	header.reason_codes.resize(1);
	for (auto ptr = header.reason_codes.data(), end = ptr + 1; ptr != end; ++ptr) {
		co_await reader.read_fields(*ptr);
	}

	co_return reader.context().return_value(ec, total_read + read);
}

} // namespace terraqtt::protocol::v5
