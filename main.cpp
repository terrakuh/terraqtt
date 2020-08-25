#include "mqtt/client.hpp"
#include "mqtt/null_mutex.hpp"
#include "mqtt/static_container.hpp"
#include "mqtt/string_view.hpp"

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

typedef mqtt::basic_client<std::istream, std::ostream, std::string,
                           mqtt::static_container<1, mqtt::protocol::suback_return_code>, mqtt::null_mutex,
                           std::chrono::steady_clock>
    client;

class my_client : public client
{
public:
	using client::client;

protected:
	void on_publish(mqtt::protocol::publish_header<string_type>& header, std::istream& payload,
	                std::size_t payload_size) override
	{
		std::cout << "received: " << payload.rdbuf() << std::endl;

		if (header.qos == mqtt::qos::at_least_once) {
			mqtt::protocol::write_packet(*output(),
			                             mqtt::protocol::puback_header{ header.packet_identifier });
		}
	}
};

int main()
{
	std::signal(SIGINT, &handler);

	asio::ip::tcp::iostream stream;

	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	my_client client{ &stream, &stream };

	client.connect(mqtt::string_view{ "der.klient" }, true, mqtt::seconds{ 10 });
	client.publish(mqtt::string_view{ "output" }, mqtt::string_view{ "hello, world" },
	               mqtt::qos::at_least_once, true);
	client.subscribe({ mqtt::subscribe_topic<mqtt::string_view>{ "input", mqtt::qos::at_most_once } });
	client.output()->flush();

	while (stream) {
		boost::system::error_code ec;

		if (const auto c = client.process_one(stream.socket().available(ec))) {
			std::cout << "processed: " << c << " bytes\n";
		}

		if (ec) {
			std::cerr << "stream error: " << ec << std::endl;
			break;
		}

		client.update_state();
		client.output()->flush();

		if (val == SIGINT) {
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
	}
}
