#pragma once

#ifndef __HRPC_DETAIL_FUNC_TRAITS_H__
#define __HRPC_DETAIL_FUNC_TRAITS_H__

#include <tuple>
#include <type_traits>

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

template <typename T> using invoke = typename T::type;

template <int N>
using is_zero =
    invoke<std::conditional<(N == 0), std::true_type, std::false_type>>;

template <int N, typename... Ts>
using nth_type = invoke<std::tuple_element<N, std::tuple<Ts...>>>;

namespace tags {

// tags for the function traits, used for tag dispatching
struct zero_arg {};
struct nonzero_arg {};
struct void_result {};
struct nonvoid_result {};

template <int N> struct arg_count_trait { typedef nonzero_arg type; };

template <> struct arg_count_trait<0> { typedef zero_arg type; };

template <typename T> struct result_trait { typedef nonvoid_result type; };

template <> struct result_trait<void> { typedef void_result type; };
} // namespace tags

//! \brief Provides a small function traits implementation that
//! works with a reasonably large set of functors.
template <typename T>
struct func_traits : func_traits<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...)> : func_traits<R (*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct func_traits<R (C::*)(Args...) const> : func_traits<R (*)(Args...)> {};

template <typename R, typename... Args> struct func_traits<R (*)(Args...)> {
    using result_type = R;
    using arg_count = std::integral_constant<std::size_t, sizeof...(Args)>;
    using args_type = std::tuple<typename std::decay<Args>::type...>;
};

template <typename T>
struct func_kind_info : func_kind_info<decltype(&T::operator())> {};

template <typename C, typename R, typename... Args>
struct func_kind_info<R (C::*)(Args...)> : func_kind_info<R (*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct func_kind_info<R (C::*)(Args...) const>
    : func_kind_info<R (*)(Args...)> {};

template <typename R, typename... Args> struct func_kind_info<R (*)(Args...)> {
    typedef typename tags::arg_count_trait<sizeof...(Args)>::type args_kind;
    typedef typename tags::result_trait<R>::type result_kind;
};

template <typename F> using is_zero_arg = is_zero<func_traits<F>::arg_count>;

template <typename F>
using is_single_arg = invoke<std::conditional<func_traits<F>::arg_count == 1,
                                              std::true_type, std::false_type>>;

template <typename F>
using is_void_result = std::is_void<typename func_traits<F>::result_type>;

} // namespace hrpc::detail

#endif // __HRPC_DETAIL_FUNC_TRAITS_H__
