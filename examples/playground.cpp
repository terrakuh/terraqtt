#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <string>
#include <terraqtt/client.hpp>
#include <terraqtt/null_mutex.hpp>
#include <terraqtt/static_container.hpp>
#include <terraqtt/string_view.hpp>
#include <thread>

using namespace boost;

static volatile sig_atomic_t val = 0;

inline void handler(int signal)
{
	val = signal;
}

typedef terraqtt::Basic_client<std::istream, std::ostream, std::string,
                               terraqtt::Static_container<1, terraqtt::protocol::Suback_return_code>,
                               terraqtt::Null_mutex, std::chrono::steady_clock>
    client;

class my_client : public client
{
public:
	using client::client;

protected:
	void on_publish(std::error_code& ec, terraqtt::protocol::Publish_header<String_type>& header,
	                std::istream& payload, std::size_t payload_size) override
	{
		std::cout << "received: " << payload.rdbuf() << std::endl;
		if (header.qos == terraqtt::QOS::at_least_once) {
			terraqtt::protocol::write_packet(*output(), ec,
			                                 terraqtt::protocol::Puback_header{ header.packet_identifier });
		}
	}
};

int main()
try {
	std::signal(SIGINT, &handler);
	asio::ip::tcp::iostream stream;
	stream.connect(asio::ip::tcp::endpoint{ asio::ip::address::from_string("192.168.178.31"), 1883 });
	if (!stream) {
		std::cerr << "failed to connect to broker\n";
		return 1;
	}

	my_client client{ &stream, &stream };
	std::error_code ec;
	client.connect(terraqtt::String_view{ "der.klient" }, false, terraqtt::Seconds{ 30 });
	// client.publish(terraqtt::String_view{ "output" }, terraqtt::String_view{ "hello, world" }, 1,
	//                terraqtt::QOS::at_least_once, true);
	client.subscribe(
	    { terraqtt::Subscribe_topic<terraqtt::String_view>{ "pc/led/color", terraqtt::QOS::at_most_once } },
	    1);
	client.output()->flush();

	while (stream) {
		std::error_code e;
		boost::system::error_code ec;

		if (const auto c = client.process_one(e, stream.rdbuf()->lowest_layer().available(ec))) {
			std::cout << "processed: " << c << " bytes (" << e << ")\n";
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
	std::cerr << "stream broken\n";
} catch (const std::system_error& e) {
	std::cerr << "exception caught (" << e.code() << "): " << e.what() << '\n';
	return 1;
}
