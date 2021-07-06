#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <sstream>
#include <terraqtt/client.hpp>
#include <terraqtt/static_container.hpp>
#include <terraqtt/string_view.hpp>

using namespace terraqtt;

using Parent = Basic_client<std::istream, std::ostream, Static_container<64, char>,
                            Static_container<1, protocol::Suback_return_code>, std::chrono::steady_clock>;

inline void print_hex(const std::string& s)
{
	for (auto c : s) {
		printf("%02x ", static_cast<int>(c));
	}
	puts("");
}

template<std::size_t N>
inline void compare(const std::string& actual, const char (&expected)[N])
{
	REQUIRE(actual.length() == N - 1);
	REQUIRE(actual.compare(0, N - 1, expected, N - 1) == 0);
}

TEST_CASE("connect")
{
	std::stringstream input;
	std::stringstream output;
	Parent client{ input, output };

	REQUIRE(output.tellp() == 0);
	std::error_code ec;
	client.connect(ec, String_view{ "name" }, true, Seconds{ 30 });
	compare(output.str(), "\x10\x10\x00\x04MQTT\x04\x02\x00\x1e\x00\x04name");
}
