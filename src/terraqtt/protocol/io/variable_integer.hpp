#pragma once

#include "../../error.hpp"
#include "../general.hpp"
#include "fwd.hpp"

namespace terraqtt::protocol::io {

template<>
struct IoContext<VariableInteger, 5> {
	constexpr static std::size_t min_size = 1;
	constexpr static std::size_t max_request = 4;

	static WriteSizeReturn write_size(VariableInteger value) noexcept
	{
		if (to_underlying(value) <= 127) {
			return { {}, 1 };
		} else if (to_underlying(value) <= 16'383) {
			return { {}, 2 };
		} else if (to_underlying(value) <= 2'097'151) {
			return { {}, 3 };
		} else if (to_underlying(value) <= 268'435'455) {
			return { {}, 4 };
		}
		return { Error::variable_integer_too_large, 0 };
	}
	static int make_write_context() noexcept { return -1; }
	static OperationReturn write(int& bytes_written, std::span<std::byte> buffer,
	                             VariableInteger value) noexcept
	{
		// First call.
		if (bytes_written == -1) {
			if (to_underlying(value) > 268'435'455) {
				return { .ec = Error::variable_integer_too_large };
			}
			bytes_written = 0;
		}

		std::size_t consumed = 0;
		auto x = to_underlying(value) >> (bytes_written * 7);
		while (consumed < buffer.size()) {
			buffer[consumed++] = static_cast<std::byte>(x % 128);
			x = x >> 7;
			++bytes_written;
			if (x == 0) {
				return { .consumed = consumed };
			}
			buffer[consumed - 1] = static_cast<std::byte>(static_cast<int>(buffer[consumed - 1]) | 128);
		}

		return {
			.consumed = consumed,
			.next_call_size = static_cast<std::size_t>(x <= 127      ? 1
			                                           : x <= 16'383 ? 2
			                                                         : 3),
		};
	}
	static std::uint32_t make_read_context() noexcept { return 1; }
	static OperationReturn read(std::uint32_t& multiplier, std::span<const std::byte> buffer,
	                            VariableInteger& value) noexcept
	{
		std::size_t consumed = 0;
		while (true) {
			if (multiplier > 128 * 128 * 128) {
				return { .ec = Error::malformed_variable_integer, .consumed = consumed };
			}

			value = static_cast<VariableInteger>((static_cast<int>(buffer[consumed]) & 127) * multiplier +
			                                     to_underlying(value));
			multiplier *= 128;

			if ((static_cast<int>(buffer[consumed++]) & 128) == 0) {
				break;
			} else if (consumed == buffer.size()) {
				return { .consumed = consumed, .next_call_size = 1 };
			}
		}
		return { .consumed = consumed };
	}
};

static_assert(Readable<VariableInteger, 5> && Writeable<VariableInteger, 5>);

} // namespace terraqtt::protocol::io
