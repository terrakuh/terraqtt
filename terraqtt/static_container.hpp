#ifndef TERRAQTT_STATIC_CONTAINER_HPP_
#define TERRAQTT_STATIC_CONTAINER_HPP_

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace terraqtt {

template<std::size_t Size, typename Type>
class static_container
{
public:
	typedef Type value_type;

	void resize(std::size_t size)
	{
		if (size > max_size()) {
			throw std::bad_alloc{};
		}

		while (size > _size) {
			push_back(Type{});
		}

		while (_size > size) {
			pop_back();
		}
	}
	void push_back(const Type& value)
	{
		if (size() == max_size()) {
			throw std::out_of_range{ "container is full" };
		}

		new (&_data[_size++]) Type{ value };
	}
	void push_back(Type&& value)
	{
		if (size() == max_size()) {
			throw std::out_of_range{ "container is full" };
		}

		new (_data + _size++) Type{ std::move(value) };
	}
	void pop_back()
	{
		if (empty()) {
			throw std::out_of_range{ "container is empty" };
		}

		reinterpret_cast<Type*>(_data + --_size)->~Type();
	}
	Type* begin() noexcept
	{
		return reinterpret_cast<Type*>(_data);
	}
	Type* end() noexcept
	{
		return reinterpret_cast<Type*>(_data + _size);
	}
	const Type* begin() const noexcept
	{
		return reinterpret_cast<const Type*>(_data);
	}
	const Type* end() const noexcept
	{
		return reinterpret_cast<const Type*>(_data + _size);
	}
	std::size_t max_size() const noexcept
	{
		return Size;
	}
	std::size_t size() const noexcept
	{
		return _size;
	}
	bool empty() const noexcept
	{
		return _size;
	}

private:
	typename std::aligned_storage<sizeof(Type), alignof(Type)>::type _data[Size];
	std::size_t _size = 0;
};

} // namespace terraqtt

#endif
