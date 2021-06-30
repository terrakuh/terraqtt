# MQTT Client
An open source, header only and fully templated MQTT Library for C++11. The main focus is to be fully generic and resource efficient. This library only provides the means to communicate with a MQTT broker and not the logic for example for QoS management (although QoS::at_most_once requires no management). This also works with the Arduino framework (was tested on a ESP8266).

Supported protocols:
- MQTT v3.1.1

The documentation can be found [here](https://terrakuh.github.io/terraqtt/docs/index.html).

## Building & Installing
Since there is nothing to compile except the examples you can run the following

```sh
mkdir build && cd build

cmake ..
cmake --build .
cmake --build . --target install
```

## CMake
After installing or adding it by `add_subdirectory` you can link the target `terraqtt::terraqtt` like so:

```cmake
find_package(terraqtt REQUIRED)

target_link_libraries(my_target terraqtt::terraqtt)
```

## TODO
- Will message
- Advanced client
