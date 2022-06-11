#ifndef TERRAQTT_PROTOCOL_READER_HPP_
#define TERRAQTT_PROTOCOL_READER_HPP_

#include "../error.hpp"
#include "general.hpp"

#include <type_traits>
#include <utility>

namespace terraqtt {
namespace protocol {

template<typename Input>
inline Control_packet_type peek_type(Input& input, std::error_code& ec, Read_context& context)
{
	if (input.good()) {
		return static_cast<Control_packet_type>(input.peek() >> 4 & 0x0f);
	}
	ec = std::make_error_code(std::errc::io_error);
	return Control_packet_type::reserved;
}

template<typename Input, typename Type>
inline typename std::enable_if<(std::is_trivial<Type>::value && sizeof(Type) == 1), bool>::type
  read_element(Input& input, std::error_code& ec, Read_context& context, Type& out)
{
	if (!context.available) {
		return false;
	} else if (!context.remaining_size) {
		ec = Error::bad_remaining_size;
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
inline bool read_element(Input& input, std::error_code& ec, Read_context& context, std::uint16_t& out)
{
	if (context.available == 0) {
		return false;
	} else if (context.remaining_size < 2 - context.sequence_data[0]) {
		ec = Error::bad_remaining_size;
		return false;
	}

	const auto buffer = reinterpret_cast<typename Input::char_type*>(context.sequence_data + 1);
	const auto n      = std::min<std::streamsize>(2 - context.sequence_data[0], context.available);
	input.read(buffer + context.sequence_data[0], n);
	context.available -= n;
	context.remaining_size -= n;
	context.sequence_data[0] += n;
	if (!input) {
		ec = std::make_error_code(std::errc::io_error);
		return false;
	} else if (context.sequence_data[0] != 2) {
		return false;
	}

	out                      = buffer[0] << 8 | buffer[1];
	context.sequence_data[0] = 0;
	context.sequence_data[1] = 0;
	++context.sequence;
	return true;
}

template<typename Input>
inline bool read_element(Input& input, std::error_code& ec, Read_context& context, Variable_integer& out)
{
	// set the multiplier
	if (!context.sequence_data[1]) {
		context.sequence_data[1] = 1;
	}
	int c = 0;

	do {
		if (!context.available) {
			return false;
		} else if (!context.remaining_size) {
			ec = Error::bad_remaining_size;
			return false;
		}

		c = input.get();
		if (!input) {
			ec = std::make_error_code(std::errc::io_error);
			return false;
		}

		--context.available;
		--context.remaining_size;
		context.sequence_data[0] += (c & 0x7f) * context.sequence_data[1];
		context.sequence_data[1] *= 128;

		if (context.sequence_data[1] > 128 * 128 * 128 && c & 0x80) {
			ec = Error::bad_remaining_size;
			return false;
		}
	} while (c & 0x80);

	out                      = static_cast<Variable_integer>(context.sequence_data[0]);
	context.sequence_data[0] = 0;
	context.sequence_data[1] = 0;
	++context.sequence;
	return true;
}

template<typename Input, typename Blob>
inline bool read_blob(Input& input, std::error_code& ec, Read_context& context, Blob& blob)
{
	std::uint16_t size = static_cast<std::uint16_t>(context.sequence_data[0]);
	if ((!size && !read_element(input, ec, context, size)) || ec) {
		return false;
	} else if (context.remaining_size < size) {
		ec = Error::bad_remaining_size;
		return false;
	}

	context.sequence_data[0] = size;
	for (; context.sequence_data[0] && context.available;
	     --context.sequence_data[0], --context.available, --context.remaining_size) {
		if (blob.size() >= blob.max_size()) {
			ec = std::make_error_code(std::errc::not_enough_memory);
			return false;
		}

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
