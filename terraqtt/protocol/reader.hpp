#ifndef TERRAQTT_PROTOCOL_READER_HPP_
#define TERRAQTT_PROTOCOL_READER_HPP_

#include "general.hpp"

namespace terraqtt {
namespace protocol {

template<typename Input>
inline control_packet_type peek_type(Input& input, read_context& context)
{
	return input.good() ? static_cast<control_packet_type>(input.peek() >> 4 & 0x0f)
	                    : throw io_error{ "failed to read" };
}

template<typename Input>
inline bool read_element(Input& input, read_context& context, byte& out)
{
	if (!context.available) {
		return false;
	} else if (!context.remaining_size) {
		throw protocol_error{ "invalid remaining size" };
	}

	input.read(reinterpret_cast<typename Input::char_type*>(&out), sizeof(out));

	if (!input) {
		throw io_error{ "failed to read" };
	}

	--context.available;
	--context.remaining_size;
	++context.sequence;

	return true;
}

template<typename Input>
inline bool read_element(Input& input, read_context& context, std::uint16_t& out)
{
	if (context.available < 2) {
		return false;
	} else if (context.remaining_size < 2) {
		throw protocol_error{ "invalid remaining size" };
	}

	typename Input::char_type buffer[2];

	input.read(buffer, sizeof(buffer));

	if (!input) {
		throw io_error{ "failed to read" };
	}

	out = buffer[0] << 8 | buffer[1];
	context.available -= 2;
	context.remaining_size -= 2;
	++context.sequence;

	return true;
}

template<typename Input>
inline bool read_element(Input& input, read_context& context, variable_integer& out)
{
	for (; context.sequence_data[1] < 4; ++context.sequence_data[1]) {
		if (!context.available) {
			return false;
		} else if (!context.remaining_size) {
			throw protocol_error{ "invalid remaining size" };
		}

		const auto c = input.get();

		if (!input) {
			throw io_error{ "failed to read" };
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
inline bool read_blob(Input& input, read_context& context, Blob& blob)
{
	std::uint16_t size = static_cast<std::uint16_t>(context.sequence_data[0]);

	if (!size && !read_element(input, context, size)) {
		return false;
	} else if (context.remaining_size < size) {
		throw protocol_error{ "invalid remaining size" };
	}

	context.sequence_data[0] = size;

	for (; context.sequence_data[0] && context.available;
	     --context.sequence_data[0], --context.available, --context.remaining_size) {
		const auto c = input.get();

		if (!input) {
			throw io_error{ "failed to read" };
		}

		blob.push_back(static_cast<typename Blob::value_type>(c));
	}

	return !context.sequence_data[0];
}
/*
template<bool ReadSize, typename Blob>
inline void read_blob(byte_istream& input, variable_integer remaining_size, Blob& blob)
{
    auto remaining = static_cast<variable_integer_type>(remaining_size);

    if (ReadSize) {
        if (remaining < 2) {
            throw protocol_error{ "invalid remaining size" };
        }

        std::uint16_t size;

        read_element(input, size);

        if (size > remaining - 2) {
            throw protocol_error{ "invalid remaining size" };
        }

        remaining = size;
    }

    while (remaining) {
        const auto c = input.get();

        if (!input) {
            throw io_error{ "could not read remaining blob" };
        }

        blob.push_back(static_cast<typename Blob::value_type>(input.get()));
    }
}*/

} // namespace protocol
} // namespace mqtt

#endif
