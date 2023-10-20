#pragma once

#include <bit>
#include <concepts>
#include <cstddef>

namespace terraqtt {

template<typename Type, typename Element = std::byte>
concept ReadableContainer = requires(const Type& cref) {
	requires std::is_same_v<typename Type::value_type, Element>;

	{
		cref.max_size()
	} -> std::same_as<std::size_t>;
	{
		cref.size()
	} -> std::same_as<std::size_t>;
	{
		cref.data()
	} -> std::same_as<const Element*>;
};

template<typename Type, typename Element = std::byte>
concept WriteableContainer = ReadableContainer<Type, Element> && requires(Type& ref, Element&& element) {
	ref.clear();
	ref.resize(std::size_t{ 0 });
	ref.push_back(element);
	{
		ref.data()
	} -> std::same_as<Element*>;
};

template<typename Type>
concept ReadableString = ReadableContainer<Type, char>;

template<typename Type>
concept WriteableString = WriteableContainer<Type, char>;

} // namespace terraqtt
