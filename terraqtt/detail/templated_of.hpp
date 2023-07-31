#pragma once

#include <type_traits>

namespace terraqtt::detail {

template<template<typename...> typename Template, typename Type>
struct TemplatedOfImpl : std::false_type {};

template<template<typename...> typename Template, typename... Types>
struct TemplatedOfImpl<Template, Template<Types...>> : std::true_type {};

template<typename Type, template<typename...> typename Template>
concept TemplatedOf = TemplatedOfImpl<Template, std::decay_t<Type>>::value;

} // namespace terraqtt::detail
