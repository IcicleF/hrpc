#pragma once

#ifndef __HRPC_DETAIL_REMOVE_FIRST_ARG_H__
#define __HRPC_DETAIL_REMOVE_FIRST_ARG_H__

#include <tuple>
#include <type_traits>

namespace hrpc::detail {

template <typename T> struct remove_first_arg;

template <typename Head, typename... Tail>
struct remove_first_arg<std::tuple<Head, Tail...>> {
    using type = std::tuple<Tail...>;
};

} // namespace hrpc::detail

#endif // __HRPC_DETAIL_REMOVE_FIRST_ARG_H__
