#ifndef MQTT_PROTOCOL_READER_HPP_
#define MQTT_PROTOCOL_READER_HPP_

#include "general.hpp"

namespace mqtt {
namespace protocol {

inline control_packet_type peek_type(byte_istream& input, read_context& context)
{
	return input.good() ? static_cast<control_packet_type>(input.peek() >> 4 & 0x0f)
	                    : throw io_error{ "failed to read" };
}

inline bool read_element(byte_istream& input, read_context& context, byte& out)
{
	if (!context.available) {
		return false;
	}

	input.read(reinterpret_cast<byte_istream::char_type*>(&out), sizeof(out));

	if (!input) {
		throw io_error{ "failed to read" };
	}

	--context.available;
	++context.sequence;

	return true;
}

inline bool read_element(byte_istream& input, read_context& context, std::uint16_t& out)
{
	if (context.available < 2) {
		return false;
	}

	byte_istream::char_type buffer[2];

	input.read(buffer, sizeof(buffer));

	if (!input) {
		throw io_error{ "failed to read" };
	}

	out = buffer[0] << 8 | buffer[1];
	context.available -= 2;
	++context.sequence;

	return true;
}

inline bool read_element(byte_istream& input, read_context& context, variable_integer& out)
{
	for (; context.sequence_data[1] < 4; ++context.sequence_data[1]) {
		if (!context.available) {
			return false;
		}

		const auto c = input.get();

		if (!input) {
			throw io_error{ "failed to read" };
		}

		--context.available;
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
