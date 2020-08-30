#ifndef TERRAQTT_PROTOCOL_READER_HPP_
#define TERRAQTT_PROTOCOL_READER_HPP_

#include "../error.h"
#include "general.hpp"

#include <utility>

namespace terraqtt {
namespace protocol {

template<typename Input>
inline control_packet_type peek_type(Input& input, std::error_code& ec, read_context& context)
{
	if (input.good()) {
		return static_cast<control_packet_type>(input.peek() >> 4 & 0x0f);
	}

	ec = std::make_error_code(std::errc::io_error);
	return control_packet_type{};
}

template<typename Input>
inline bool read_element(Input& input, std::error_code& ec, read_context& context, byte& out)
{
	if (!context.available) {
		return false;
	} else if (!context.remaining_size) {
		ec = errc::bad_remaining_size;
		return false;
	}

	input.read(reinterpret_cast<typename Input::char_type*>(&out), sizeof(out));

	if (!input) {
		ec = std::make_error_code(std::errc::io_error);
		return false;
	}

	--context.available;
	--context.remaining_size;
	++context.sequence;

	return true;
}

template<typename Input>
inline bool read_element(Input& input, std::error_code& ec, read_context& context, std::uint16_t& out)
{
	if (context.available < 2) {
		return false;
	} else if (context.remaining_size < 2) {
		ec = errc::bad_remaining_size;
		return false;
	}

	typename Input::char_type buffer[2];

	input.read(buffer, sizeof(buffer));

	if (!input) {
		ec = std::make_error_code(std::errc::io_error);
		return false;
	}

	out = buffer[0] << 8 | buffer[1];
	context.available -= 2;
	context.remaining_size -= 2;
	++context.sequence;

	return true;
}

template<typename Input>
inline bool read_element(Input& input, std::error_code& ec, read_context& context, variable_integer& out)
{
	for (; context.sequence_data[1] < 4; ++context.sequence_data[1]) {
		if (!context.available) {
			return false;
		} else if (!context.remaining_size) {
			ec = errc::bad_remaining_size;
			return false;
		}

		const auto c = input.get();

		if (!input) {
			ec = std::make_error_code(std::errc::io_error);
			return false;
		}

		--context.available;
		--context.remaining_size;
		context.sequence_data[0] = context.sequence_data[0] << 7 | (c & 0x7f);

		if (!(c & 0x80)) {
			break;
		}
	}

	out                      = static_cast<variable_integer>(context.sequence_data[0]);
	context.sequence_data[0] = 0;
	context.sequence_data[1] = 0;
	++context.sequence;

	return true;
}

template<typename Input, typename Blob>
inline bool read_blob(Input& input, std::error_code& ec, read_context& context, Blob& blob)
{
	std::uint16_t size = static_cast<std::uint16_t>(context.sequence_data[0]);

	if (!size && !read_element(input, ec, context, size) || ec) {
		return false;
	} else if (context.remaining_size < size) {
		ec = errc::bad_remaining_size;
		return false;
	}

	context.sequence_data[0] = size;

	for (; context.sequence_data[0] && context.available;
	     --context.sequence_data[0], --context.available, --context.remaining_size) {
		const auto c = input.get();

		if (!input) {
			ec = std::make_error_code(std::errc::io_error);
			return false;
		}

		blob.push_back(static_cast<typename Blob::value_type>(c));
	}

	return !context.sequence_data[0];
}

} // namespace protocol
} // namespace terraqtt

#endif
