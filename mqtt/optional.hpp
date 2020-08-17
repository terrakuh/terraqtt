/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
#pragma once

#include <exception>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace optional_detail {

template<typename... Types>
struct make_void
{
    typedef void type;
};

template<typename... Types>
using void_type = typename make_void<Types...>::type;

template<typename Function, typename = void, typename... Arguments>
struct function_result;

template<typename Return, typename... Arguments>
struct function_result<Return (*)(Arguments...), void, Arguments...>
{
    typedef Return type;
};

template<typename Return, typename Class, typename... Arguments>
struct function_result<Return (Class::*)(Arguments...), void, Arguments...>
{
    typedef Return type;
};

template<typename Return, typename Class, typename... Arguments>
struct function_result<Return (Class::*)(Arguments...) const, void, Arguments...>
{
    typedef Return type;
};

template<typename Functor, typename... Arguments>
struct function_result<Functor, void_type<decltype(std::declval<Functor>()(std::declval<Arguments>()...))>,
                       Arguments...>
{
    typedef decltype(std::declval<Functor>()(std::declval<Arguments>()...)) type;
};

template<typename Function, typename... Arguments>
using function_result_type = typename function_result<Function, void, Arguments...>::type;

class no_such_element_error : public std::runtime_error
{
public:
    no_such_element_error() noexcept : std::runtime_error("no element present in optional")
    {}
};

template<typename T>
class optional;

template<typename T>
inline optional<T> make_optional(T&& _value)
{
    return { std::forward<T>(_value) };
}

template<>
class optional<void>
{
public:
    bool present() const noexcept
    {
        return false;
    }
    bool empty() const noexcept
    {
        return true;
    }
};

template<typename T>
class optional
{
public:
    typedef T value_type;

    optional() noexcept
    {
        _present = false;
    }
    template<typename... Args>
    optional(Args&&... args)
    {
        _present = false;

        emplace(std::forward<Args>(args)...);
    }
    optional(const optional& copy)
    {
        if (copy._present) {
            new (&_data) T(copy.get());
        }

        _present = copy._present;
    }
    optional(optional&& move)
    {
        if (move._present) {
            new (&_data) T(std::move(move.get()));

            _present      = true;
            move._present = false;
        } else {
            _present = false;
        }
    }
    ~optional()
    {
        reset();
    }
    void reset()
    {
        if (_present) {
            _present = false;

            reinterpret_cast<T*>(&_data)->~T();
        }
    }
    template<typename Consumer, typename... Args>
    void if_present(Consumer&& consumer, Args&&... args)
    {
        if (_present) {
            consumer(get(), std::forward<Args>(args)...);
        }
    }
    template<typename Consumer, typename... Args>
    void if_present(Consumer&& consumer, Args&&... args) const
    {
        if (_present) {
            consumer(get(), std::forward<Args>(args)...);
        }
    }
    template<typename Consumer, typename Runnable, typename... Consumer_args, typename... Runnable_args>
    void if_present_or_else(Consumer&& consumer, Runnable&& runnable, Consumer_args&&... consumer_args,
                            Runnable_args&&... runnable_args)
    {
        if (_present) {
            consumer(get(), std::forward<Consumer_args>(consumer_args)...);
        } else {
            runnable(std::forward<Runnable_args>(runnable_args)...);
        }
    }
    template<typename Consumer, typename Runnable, typename... Consumer_args, typename... Runnable_args>
    void if_present_or_else(Consumer&& consumer, Runnable&& runnable, Consumer_args&&... consumer_args,
                            Runnable_args&&... runnable_args) const
    {
        if (_present) {
            consumer(get(), std::forward<Consumer_args>(consumer_args)...);
        } else {
            runnable(std::forward<Runnable_args>(runnable_args)...);
        }
    }
    template<typename... Args>
    void emplace(Args&&... args)
    {
        reset();

        new (&_data) T(std::forward<Args>(args)...);

        _present = true;
    }
    bool present() const noexcept
    {
        return _present;
    }
    bool empty() const noexcept
    {
        return !_present;
    }
    T& get()
    {
        if (!_present) {
            throw no_such_element_error();
        }

        return *reinterpret_cast<T*>(&_data);
    }
    const T& get() const
    {
        if (!_present) {
            throw no_such_element_error();
        }

        return *reinterpret_cast<const T*>(&_data);
    }
    T& or_else(T& value)
    {
        if (_present) {
            return get();
        }

        return value;
    }
    T or_else(T&& value)
    {
        if (_present) {
            return get();
        }

        return value;
    }
    const T& or_else(const T& value) const
    {
        if (_present) {
            return get();
        }

        return value;
    }
    template<typename Supplier>
    typename std::conditional<std::is_reference<function_result_type<Supplier>>::value, T&, T>::type
        or_else_get(Supplier&& supplier)
    {
        if (_present) {
            return get();
        }

        return supplier();
    }
    template<typename Supplier>
    typename std::conditional<std::is_reference<function_result_type<Supplier>>::value, const T&, T>::type
        or_else_get(Supplier&& supplier) const
    {
        if (_present) {
            return get();
        }

        return supplier();
    }
    T& or_else_throw(std::exception_ptr e)
    {
        if (_present) {
            return get();
        }

        std::rethrow_exception(e);
    }
    const T& or_else_throw(std::exception_ptr e) const
    {
        if (_present) {
            return get();
        }

        std::rethrow_exception(e);
    }
    template<typename Exception, typename... Args>
    T& or_else_throw(Args&&... args)
    {
        if (_present) {
            return get();
        }

        throw Exception(std::forward<Args>(args)...);
    }
    template<typename Exception, typename... Args>
    const T& or_else_throw(Args&&... args) const
    {
        if (_present) {
            return get();
        }

        throw Exception(std::forward<Args>(args)...);
    }

    template<typename Return, typename Ty = T, typename... Args>
    typename std::enable_if<std::is_class<Ty>::value, optional>::type filter(Return (Ty::*filter)(Args...),
                                                                             Args&&... args)
    {
        if (_present && (get().*filter(std::forward<Args>(args)...))) {
            return *this;
        }

        return {};
    }
    template<typename Return, typename Ty = T, typename... Args>
    typename std::enable_if<std::is_class<Ty>::value, optional>::type filter(Return (Ty::*filter)(Args...),
                                                                             Args&&... args) const
    {
        if (_present && (get().*filter(std::forward<Args>(args)...))) {
            return *this;
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional filter(Return (*filter)(T&, Args...), Args&&... args)
    {
        if (_present && filter(get(), std::forward<Args>(args)...)) {
            return *this;
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional filter(Return (*filter)(T, Args...), Args&&... args) const
    {
        if (_present && filter(get(), std::forward<Args>(args)...)) {
            return *this;
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional filter(Return (*filter)(const T&, Args...), Args&&... args) const
    {
        if (_present && filter(get(), std::forward<Args>(args)...)) {
            return *this;
        }

        return {};
    }
    template<typename Filter, typename... Args>
    optional filter(Filter&& filter, Args&&... args)
    {
        if (_present && filter(get(), std::forward<Args>(args)...)) {
            return *this;
        }

        return {};
    }
    template<typename Filter, typename... Args>
    optional filter(Filter&& filter, Args&&... args) const
    {
        if (_present && filter(get(), std::forward<Args>(args)...)) {
            return *this;
        }

        return {};
    }

    template<typename Return, typename Ty = T, typename... Args>
    typename std::enable_if<std::is_class<Ty>::value, optional<Return>>::type map(Return (Ty::*mapper)(Args...),
                                                                                  Args&&... args)
    {
        if (_present) {
            return { (get().*mapper)(std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Return, typename Ty = T, typename... Args>
    typename std::enable_if<std::is_class<Ty>::value, optional<Return>>::type map(Return (Ty::*mapper)(Args...) const,
                                                                                  Args&&... args) const
    {
        if (_present) {
            return { (get().*mapper)(std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional<Return> map(Return (*mapper)(T&, Args...), Args&&... args)
    {
        if (_present) {
            return { mapper(get(), std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional<Return> map(Return (*mapper)(T, Args...), Args&&... args) const
    {
        if (_present) {
            return { mapper(get(), std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Return, typename... Args>
    optional<Return> map(Return (*mapper)(const T&, Args...), Args&&... args) const
    {
        if (_present) {
            return { mapper(get(), std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Mapper, typename... Args>
    auto map(Mapper&& mapper, Args&&... args) -> decltype(make_optional(mapper(get(), std::forward<Args>(args)...)))
    {
        if (_present) {
            return { mapper(get(), std::forward<Args>(args)...) };
        }

        return {};
    }
    template<typename Mapper, typename... Args>
    auto map(Mapper&& mapper, Args&&... args) const
        -> decltype(make_optional(mapper(get(), std::forward<Args>(args)...)))
    {
        if (_present) {
            return { mapper(get(), std::forward<Args>(args)...) };
        }

        return {};
    }

    operator bool() const noexcept
    {
        return present();
    }
    operator T&()
    {
        return get();
    }
    operator const T&() const
    {
        return get();
    }
    T* operator->()
    {
        return &get();
    }
    const T* operator->() const
    {
        return &get();
    }
    optional& operator=(const optional& copy)
    {
        reset();

        if (copy._present) {
            new (&_data) T(copy.get());

            _present = true;
        }

        return *this;
    }
    optional& operator=(optional&& move)
    {
        reset();

        if (move._present) {
            new (&_data) T(std::move(move.get()));

            _present      = true;
            move._present = false;
        }

        return *this;
    }

private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type _data;
    bool _present;
};

} // namespace optional_detail

using no_such_element_error = optional_detail::no_such_element_error;

template<typename T>
using optional = typename optional_detail::optional<T>;

using optional_detail::make_optional;

namespace std {

template<typename T>
struct hash<optional<T>>
{
    std::size_t operator()(const optional<T>& _value) const
    {
        return _value ? std::hash<T>()(_value.get()) : 0;
    }
};

} // namespace std
