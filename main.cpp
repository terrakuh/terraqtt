#include "terraqtt/client.hpp"
#include "terraqtt/null_mutex.hpp"
#include "terraqtt/static_container.hpp"
#include "terraqtt/string_view.hpp"

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

typedef terraqtt::basic_client<std::istream, std::ostream, std::string,
                           terraqtt::static_container<1, terraqtt::protocol::suback_return_code>, terraqtt::null_mutex,
                           std::chrono::steady_clock>
    client;

class my_client : public client
{
public:
	using client::client;

protected:
	void on_publish(terraqtt::protocol::publish_header<string_type>& header, std::istream& payload,
	                std::size_t payload_size) override
	{
		std::cout << "received: " << payload.rdbuf() << std::endl;

		if (header.qos == terraqtt::qos::at_least_once) {
			terraqtt::protocol::write_packet(*output(),
			                             terraqtt::protocol::puback_header{ header.packet_identifier });
		}
	}
};

int main()
{
	std::signal(SIGINT, &handler);

	asio::ip::tcp::iostream stream;

	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	my_client client{ &stream, &stream };

	client.connect(terraqtt::string_view{ "der.klient" }, true, terraqtt::seconds{ 10 });
	client.publish(terraqtt::string_view{ "output" }, terraqtt::string_view{ "hello, world" },
	               terraqtt::qos::at_least_once, true);
	client.subscribe({ terraqtt::subscribe_topic<terraqtt::string_view>{ "input", terraqtt::qos::at_most_once } });
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
