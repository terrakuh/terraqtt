#include "mqtt/client.hpp"

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

	client.connect("der.klient", true, mqtt::seconds{ 0 });
	client.publish(std::string{ "output" }, std::vector<char>{ 'h', 'e', 'l', 'l', 'o', 0 }, mqtt::qos::at_least_once);
	client.subscribe(
	    { mqtt::subscribe_topic<std::string>{ "input", mqtt::qos::at_least_once } });

	while (true) {
		std::cout << "processed: " << client.process_one() << std::endl;
		/*if (const auto c = client.process_one(stream.rdbuf()->available())) {
			std::cout << "processed: " << c << "\n";
		}

		if (val == SIGINT) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });*/
	}
}
