#pragma once

#include "write_fields.hpp"

#include <cstring>

namespace terraqtt::protocol::io {

template<int Version, AsyncContext Context, std::size_t BufferSize = TERRAQTT_BUFFER_SIZE>
class Writer {
public:
	Writer(Context& context) : _context{ context } {}

	template<Writeable<Version>... Types>
	typename Context::template return_type<std::size_t> write_fields(const Types&... fields)
	{
		std::error_code ec{};
		std::size_t total_written = 0;

		std::variant<std::monostate, decltype(IoContext<Version, Types>::make_read_context())...> contexts{};
		auto fields_tuple = std::forward_as_tuple(fields...);

		std::size_t index = 0;
		std::size_t required_size = 0;
		while (!ec && index != sizeof...(fields)) {
			if (required_size > _buffer.size()) {
				std::size_t written = 0;
				std::tie(ec, written) = co_await flush();
				total_written += written;
			}

			_write_fields_impl(ec, index, required_size, contexts, fields...);
		}

		co_return _context.return_value(ec, total_written);
	}
	typename Context::template return_type<std::size_t> flush()
	{
		std::error_code ec{};
		std::size_t written = 0;
		if (sizeof(_static_buffer) != _buffer.size()) {
			std::tie(ec, written) = co_await _context.write({ _static_buffer, _buffer.data() });
			assert(ec || written == _buffer.data() - _static_buffer);
			_buffer = { _static_buffer };
		}
		co_return _context.return_value(ec, written);
	}

private:
	Context& _context;
	std::byte _static_buffer[BufferSize];
	std::span<std::byte> _buffer{ _static_buffer };

	template<typename Contexts, Writeable<Version>... Types, std::size_t... Indices>
	void _write_fields_impl(std::error_code& ec, std::size_t& index, std::size_t& required_size,
	                        Contexts& contexts, const Types&... fields,
	                        std::index_sequence<Indices...> = std::index_sequence_for<Types...>{})
	{
		std::size_t current_index = 0;
		((!ec && index != current_index++ ? static_cast<void>(0) : static_cast<void>([&] {
			 using IoContext_ = IoContext<Version, Types>;

			 if (contexts.index() + 1 != Indices) {
				 contexts.template emplace<Indices>(IoContext_::make_write_context());
				 required_size = IoContext_::min_size;
			 }
			 if (_buffer.size() < required_size) {
				 return;
			 }

			 auto w = IoContext_::write(std::get<Indices>(context), _buffer, fields);
			 ec = w.ec;
			 _buffer.subspan(w.consumed);

			 // Proceed to next field.
			 if ((required_size = std::min(w.next_call_size, IoContext_::max_request)) == 0) {
				 ++index;
			 }
		 }())),
		 ...);
	}
};

} // namespace terraqtt::protocol::io
