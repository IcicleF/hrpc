#pragma once

#ifndef __HRPC_DETAIL_CALL_H__
#define __HRPC_DETAIL_CALL_H__

#include <tuple>

// https://github.com/rpclib/rpclib
//
// Copyright (c) 2015-2017, Tamás Szelei
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
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
