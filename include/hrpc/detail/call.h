#pragma once

#ifndef __HRPC_DETAIL_CALL_H__
#define __HRPC_DETAIL_CALL_H__

#include <tuple>

namespace hrpc::detail {

//! \brief Calls a functor with argument provided directly
template <typename Functor, typename Arg>
auto call(Functor f, Arg &&arg) -> decltype(f(std::forward<Arg>(arg))) {
    return f(std::forward<Arg>(arg));
}

template <typename Functor, typename... Args, std::size_t... I>
decltype(auto) call_helper(Functor func, std::tuple<Args...> &&params,
                           std::index_sequence<I...>) {
    return func(std::get<I>(params)...);
}

//! \brief Calls a functor with arguments provided as a tuple
template <typename Functor, typename... Args>
decltype(auto) call(Functor f, std::tuple<Args...> &args) {
    return call_helper(f, std::forward<std::tuple<Args...>>(args),
                       std::index_sequence_for<Args...>{});
}

} // namespace hrpc::detail

#endif // __HRPC_DETAIL_CALL_H__
