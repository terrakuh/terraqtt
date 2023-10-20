#ifndef TERRAQTT_VARIANT_HPP_
#define TERRAQTT_VARIANT_HPP_

#include "error.hpp"

#include <cstddef>
#include <limits>
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
struct Select_type : Select_type<Index - 1, Types...> {};

template<typename Type, typename... Types>
struct Select_type<0, Type, Types...> {
	typedef Type type;
};

} // namespace detail

/**
 * A variant class capable of holding any of the given types. This variant can be empty.
 *
 * @private
 * @tparam Types All allowed types of the variant.
 */
template<typename... Types>
class Variant {
public:
	constexpr static std::size_t npos = std::numeric_limits<std::size_t>::max();

	/// Destructor. Calls the destructor of the containing object if there is any.
	~Variant() noexcept { clear(); }
	/// Calls the destructor of the containing object or does nothing if this variant is empty.
	void clear() noexcept
	{
		if (_index != npos) {
			_clear<0, Types...>();
			_index = npos;
		}
	}
	/**
	 * Emplaces a new object inside the variant. If there is already one, it is destroyed. If the constructor
	 * of the new types throws, this object will be empty.
	 *
	 * @exception Anything the constructor throws.
	 * @param args The arguments passed to the constructor of the desired type.
	 * @tparam Index The index of the type given in the `Types` list.
	 * @tparam Args The argument types.
	 */
	template<std::size_t Index, typename... Args>
	void emplace(Args&&... args)
	{
		static_assert(Index < sizeof...(Types), "Given type index is out of range.");

		clear();
		new (&_data) typename detail::Select_type<Index, Types...>::type{ std::forward<Args>(args)... };
		_index = Index;
	}
	/**
	 * Returns a pointer to the containing value.
	 *
	 * @param[out] ec Contains an error if this variant is empty or the type mismatches.
	 * @returns A pointer if the type matches the desired type, otherwise `nullptr`.
	 */
	template<std::size_t Index>
	typename detail::Select_type<Index, Types...>::type* get(std::error_code& ec) noexcept
	{
		if (empty() || _index != Index) {
			ec = Error::bad_variant_cast;
			return nullptr;
		}
		return reinterpret_cast<typename detail::Select_type<Index, Types...>::type*>(&_data);
	}
	/// Returns the currently type index or `npos` if this variant is empty.
	std::size_t index() const noexcept { return _index; }
	bool empty() const noexcept { return _index >= npos; }

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
