cmake_minimum_required(VERSION 3.1)

find_package(Boost 1.65 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(lightweight lightweight.cpp)
target_link_libraries(lightweight
	${PROJECT_NAME}
	Boost::boost
	Boost::system
	Threads::Threads)
# target_compile_options(lightweight PRIVATE -Wall -Wextra -pedantic -Werror -Wno-missing-field-initializers -Wno-unused-parameter)
