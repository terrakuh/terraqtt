find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  asio
  URL https://sourceforge.net/projects/asio/files/asio/1.28.0%20%28Stable%29/asio-1.28.0.zip/download
  URL_HASH SHA256=3df3ecc218cc8c9d68a02c073846c5047412f304e4df965a91a894f5875bbfbb
)
FetchContent_MakeAvailable(asio)

add_executable(lightweight_example lightweight.cpp)
target_include_directories(lightweight_example PRIVATE "${asio_SOURCE_DIR}/include")
target_link_libraries(lightweight_example PRIVATE ${PROJECT_NAME} Threads::Threads)
target_compile_features(lightweight_example PRIVATE cxx_std_20)
target_compile_options(lightweight_example PRIVATE -fsanitize=address -save-temps)
target_link_options(lightweight_example PRIVATE -fsanitize=address)
