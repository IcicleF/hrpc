#pragma once

#ifndef __HRPC_DETAIL_SERDES_H__
#define __HRPC_DETAIL_SERDES_H__

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace hrpc::detail {

namespace serdes {

template <int N, std::size_t Offset, typename Args>
void serialize(uint8_t *buf, Args const &args) {
    if constexpr (N < std::tuple_size_v<Args>) {
        using Elem = std::decay_t<std::tuple_element_t<N, Args>>;
        struct tuple_layout {
            uint8_t padding[Offset];
            Elem elem;
        };
        size_t constexpr offset = offsetof(tuple_layout, elem);
        *reinterpret_cast<Elem *>(buf + offset) = std::get<N>(args);
        serialize<N + 1, offset + sizeof(Elem)>(buf, args);
    }
}

template <int N, std::size_t Offset, typename Args>
auto deserialize(uint8_t const *buf) {
    if constexpr (N < std::tuple_size_v<Args>) {
        using Elem = std::decay_t<std::tuple_element_t<N, Args>>;
        static_assert(std::is_trivial_v<Elem>, "Element type must be trivial");
        struct tuple_layout {
            uint8_t padding[Offset];
            Elem elem;
        };
        size_t constexpr offset = offsetof(tuple_layout, elem);
        Elem elem = *reinterpret_cast<Elem const *>(buf + offset);
        auto tail = deserialize<N + 1, offset + sizeof(Elem), Args>(buf);
        return std::tuple_cat(std::make_tuple(elem), tail);
    } else {
        return std::tuple<>{};
    }
}

} // namespace serdes

template <typename... T>
void serialize(uint8_t *buf, std::tuple<T...> const &args) {
    serdes::serialize<0, 0>(buf, args);
}

template <typename Args> Args deserialize(uint8_t const *buf) {
    return serdes::deserialize<0, 0, Args>(buf);
}

} // namespace hrpc::detail

#endif // __HRPC_DETAIL_SERDES_H__
