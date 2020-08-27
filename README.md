# MQTT Client

An open source, header only and fully templated MQTT Library for C++11.

## Example

```cpp
#include <terraqtt/client.hpp>
#include <boost/asio.hpp>
#include <string>

int main()
{
	boost::asio::ip::tcp::iostream stream;

	stream.connect(boost::asio::ip::tcp::endpoint{ asio::ip::address::from_string("127.0.0.1"), 1883 });

	terraqtt::client client{ stream, stream };

	client.connect("der.klient");
	client.publish(std::string{ "output" }, std::vector<char>{ 'h', 'e', 'l', 'l', 'o', 0 });
	client.subscribe(
	    { terraqtt::subscribe_topic<std::string>{ "input", terraqtt::qos::at_most_once } });

	while (true) {
		client.process_one();
	}
}
```

And its `CMakeLists.txt`:

```cmake
find_package(terraqtt REQUIRED)

target_link_libraries(my_target terraqtt::terraqtt)
```
