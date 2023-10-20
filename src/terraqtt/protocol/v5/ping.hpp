#pragma once

#include "../general.hpp"
#include "../io.hpp"

namespace terraqtt::protocol::v5 {

struct PingRequestHeader {};

template<io::AsyncContext Context>
inline typename Context::template return_type<std::size_t> write(io::Writer<5, Context>& writer,
                                                                 PingRequestHeader<5> /* header */)
{
	co_return co_await writer.write_fields(io::make_first_byte(ControlPacketType::ping_request, 0),
	                                       VariableInteger{ 0 });
}

template<io::AsyncContext Context>
inline typename Context::template return_type<std::size_t> read(io::Reader<5, Context>& reader,
                                                                PingRequestHeader<5>& /* header */)
{
	co_return reader.context().return_value({}, std::size_t{ 0 });
}

struct PingResponseHeader {};

template<io::AsyncContext Context>
inline typename Context::template return_type<std::size_t> write(Context& context,
                                                                 PingResponseHeader<5> /* header */)
{
	co_return co_await writer.write_fields(io::make_first_byte(ControlPacketType::ping_response, 0),
	                                       VariableInteger{ 0 });
}

template<bool Conservative, io::AsyncContext Context>
inline typename Context::template return_type<std::size_t> read(io::Reader<5, Context>& reader,
                                                                PingResponseHeader<5>& /* header */)
{
	co_return reader.context().return_value({}, std::size_t{ 0 });
}

} // namespace terraqtt::protocol::v5
