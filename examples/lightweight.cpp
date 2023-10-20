#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <terraqtt/protocol/v5/connect.hpp>
#include <terraqtt/protocol/v5/connect_acknowledge.hpp>
#include <terraqtt/protocol/v5/ping.hpp>

using namespace asio;

struct AsyncContext {
	template<typename Type>
	using return_type = awaitable<std::pair<std::error_code, Type>>;

	ip::tcp::socket& socket;
	std::ofstream rf{ "/home/yunus/Projects/terraqtt/read.bin", std::ios::binary | std::ios::out };
	std::ofstream wf{ "/home/yunus/Projects/terraqtt/write.bin", std::ios::binary | std::ios::out };

	awaitable<std::pair<std::error_code, std::size_t>> read(std::span<std::byte> buffer)
	{
		const auto [ec, read] = co_await async_read(socket, asio::buffer(buffer), as_tuple(use_awaitable));
		rf.write(reinterpret_cast<const char*>(buffer.data()), read);
		rf.flush();
		std::cout << "ec read: " << ec.message() << " of " << read << " bytes\n";
		co_return std::make_pair(ec, read);
	}
	awaitable<std::pair<std::error_code, std::size_t>> read_some(std::span<std::byte> buffer)
	{
		const auto [ec, read] = co_await socket.async_read_some(asio::buffer(buffer), as_tuple(use_awaitable));
		rf.write(reinterpret_cast<const char*>(buffer.data()), read);
		rf.flush();
		std::cout << "ec read some: " << ec.message() << " of " << read << " bytes\n";
		co_return std::make_pair(ec, read);
	}
	awaitable<std::pair<std::error_code, std::size_t>> write(std::span<const std::byte> buffer)
	{
		const auto [ec, written] = co_await async_write(socket, asio::buffer(buffer), as_tuple(use_awaitable));
		wf.write(reinterpret_cast<const char*>(buffer.data()), written);
		wf.flush();
		std::cout << "ec write: " << ec.message() << " of " << written << " bytes\n";
		co_return std::make_pair(ec, written);
	}
	template<typename Type>
	auto return_value(std::error_code ec, Type&& value)
	{
		return std::make_pair(ec, std::forward<Type>(value));
	}
	template<typename Type>
	auto return_value(std::error_code ec)
	{
		return std::make_pair(ec, Type{});
	}
};

awaitable<void> asio_main(ip::tcp::endpoint endpoint)
{
	const auto executor = co_await this_coro::executor;

	ip::tcp::socket socket{ executor };
	co_await socket.async_connect(endpoint, use_awaitable);
	std::cout << "Connected!\n";

	AsyncContext context{ socket };

	terraqtt::protocol::v5::ConnectHeader<std::string_view> header{
		/* .receive_maximum = 1, .maximum_packet_size = 1024 */ .client_identifier = "terraqtt"
	};
	co_await terraqtt::protocol::v5::write(context, header);
	std::cout << "Connect written\n";

	terraqtt::protocol::v5::ConnectAcknowledgeHeader<std::string> ack{};
	terraqtt::protocol::io::Reader<5, false, AsyncContext> reader{ context };
	co_await reader.read_fixed_header();
	co_await terraqtt::protocol::v5::read(reader, ack);

	std::cout << "Done: " << static_cast<int>(ack.reason_code) << "\n";
	std::cout << "Topic alias: " << ack.topic_alias_maximum << "\n";

	co_return;
}

int main()
{
	io_service service{};
	co_spawn(service, asio_main(ip::tcp::endpoint{ ip::address::from_string("192.168.178.31"), 1883 }),
	         detached);
	service.run();
}
