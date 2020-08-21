#include "mqtt/client.hpp"
#include "mqtt/protocol/connection.hpp"
#include "mqtt/protocol/ping.hpp"
#include "mqtt/protocol/publishing.hpp"
#include "mqtt/protocol/subscription.hpp"

#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <string>

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
	asio::ip::tcp::iostream stream;

	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	using namespace mqtt::protocol;
	connect_header<std::string, std::string, std::string> header{};

	header.client_identifier = "mqtt-client";
	header.clean_session     = true;
	header.keep_alive        = 60;
	std::string user         = "me";
	header.username          = &user;

	write_packet(stream, header);

	publish_header<std::string, std::string> h{};

	h.topic   = "/msg";
	h.payload = "hi";

	write_packet(stream, h);
	write_packet(stream, pingreq_header{});

	while (true) {
		if (val == SIGINT) {
			write_packet(stream, disconnect_header{});

			break;
		}

		service.run_one();
	}
}
