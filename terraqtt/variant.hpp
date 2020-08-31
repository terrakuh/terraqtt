#ifndef TERRAQTT_VARIANT_HPP_
#define TERRAQTT_VARIANT_HPP_

#include "error.hpp"

#include <type_traits>
#include <utility>

namespace terraqtt {
namespace detail {

template<typename Type>
constexpr std::size_t max_size_of() noexcept
{
	return sizeof(Type);
}

template<typename Type, typename... Other>
constexpr typename std::enable_if<(sizeof...(Other) > 0), std::size_t>::type max_size_of() noexcept
{
	return sizeof(Type) >= max_size_of<Other...>() ? sizeof(Type) : max_size_of<Other...>();
}

template<typename Type>
constexpr std::size_t max_align_of() noexcept
{
	return alignof(Type);
}

template<typename Type, typename... Other>
constexpr typename std::enable_if<(sizeof...(Other) > 0), std::size_t>::type max_align_of() noexcept
{
	return alignof(Type) >= max_align_of<Other...>() ? alignof(Type) : max_align_of<Other...>();
}

template<std::size_t Index, typename Type, typename... Types>
struct select_type : select_type<Index - 1, Types...>
{};

template<typename Type, typename... Types>
struct select_type<0, Type, Types...>
{
	typedef Type type;
};

} // namespace detail

template<typename... Types>
class variant
{
public:
	constexpr static auto npos = sizeof...(Types);

	~variant()
	{
		clear();
	}
	void clear() noexcept
	{
		if (_index < npos) {
			_clear<0, Types...>();

			_index = npos;
		}
	}
	template<std::size_t Index, typename... Args>
	void emplace(Args&&... args)
	{
		static_assert(Index < npos, "type index out of range");

		clear();

		new (&_data) typename detail::select_type<Index, Types...>::type{ std::forward<Args>(args)... };

		_index = Index;
	}
	template<std::size_t Index>
	typename detail::select_type<Index, Types...>::type* get(std::error_code& ec) noexcept
	{
		if (empty() || _index != Index) {
			ec = errc::bad_variant_cast;
			return nullptr;
		}

		return reinterpret_cast<typename detail::select_type<Index, Types...>::type*>(&_data);
	}
	std::size_t index() const noexcept
	{
		return _index;
	}
	bool empty() const noexcept
	{
		return _index >= npos;
	}

private:
	typename std::aligned_storage<detail::max_size_of<Types...>(), detail::max_align_of<Types...>()>::type
	    _data;
	std::size_t _index = npos;

	template<std::size_t Index>
	void _clear() noexcept
	{}
	template<std::size_t Index, typename Type, typename... Other>
	void _clear() noexcept
	{
		if (_index == Index) {
			reinterpret_cast<Type*>(&_data)->~Type();
		} else {
			_clear<Index + 1, Other...>();
		}
	}
};

} // namespace terraqtt

#endif
