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

	asio::ip::tcp::iostream stream;

	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	mqtt::client client{ stream, stream };

	client.connect("der.klient");
	client.publish(std::string{ "/msg" }, std::vector<char>{ 'h', 'e', 'l', 'l', 'o', 0 });

	while (true) {
		std::cout << "processed: " << client.process_one(stream.rdbuf()->available()) << "\n";

		if (val == SIGINT) {
			client.ping();
			val = 0;
		}

		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
	}
}
