#include "mqtt/detail/connect.hpp"
#include "mqtt/detail/publish.hpp"

#include <boost/asio.hpp>
#include <csignal>
#include <iostream>

using namespace boost;

static volatile sig_atomic_t val = 0;

inline void handler(int signal)
{
	val = signal;
}

int main()
{
	std::signal(SIGINT, &handler);

	asio::io_service service;
	asio::ip::tcp::iostream stream{ asio::ip::tcp::socket{ service } };

	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	using namespace mqtt::detail;
	mqtt::detail::connect_header header{};

	header.client_identifier.first  = "mwtst";
	header.client_identifier.second = 4;
	header.clean_session            = true;
	header.keep_alive               = 60;

	mqtt::detail::write_packet(stream, header);

	mqtt::detail::publish_header h{};

	h.topic.first    = "/msg";
	h.topic.second   = 4;
	h.payload.first  = (const mqtt::detail::byte*) "hi";
	h.payload.second = 3;

	mqtt::detail::write_packet(stream, h);
	mqtt::detail::write_elements(stream,
	                             static_cast<byte>(static_cast<int>(control_packet_type::pingreq) << 4),
	                             static_cast<variable_integer>(0));

	while (true) {
		if (val == SIGINT) {
			write_elements(stream, static_cast<byte>(static_cast<int>(control_packet_type::disconnect) << 4),
			               static_cast<variable_integer>(0));

			break;
		}

		service.run_one();
	}
}
