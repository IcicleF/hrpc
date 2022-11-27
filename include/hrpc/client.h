#pragma once

#ifndef __HRPC_CLIENT_H__
#define __HRPC_CLIENT_H__

#include <asio.hpp>
#include <string>
#include <string_view>
#include <type_traits>

#include "common.h"
#include "detail/serdes.h"

namespace hrpc {

class client {
public:
    client(std::string_view addr, uint16_t port) : socket(io_ctx) {
        asio::ip::tcp::endpoint ep = {
            asio::ip::address::from_string(addr.data()), port};
        socket.connect(ep);
    }

    ~client() {
        asio::error_code err;
        socket.close(err);
    }

    template <typename R, typename... Args> R call(hrpc_id_t id, Args... args) {
        // Send the request to server
        uint8_t *buf = new uint8_t[sizeof(std::tuple<Args...>)];
        detail::serialize(buf, std::forward_as_tuple(args...));
        socket.send(asio::buffer(&id, sizeof(id)));
        socket.send(asio::buffer(buf, sizeof(std::tuple<Args...>)));
        delete[] buf;

        // Receive response
        if constexpr (std::is_void_v<R>) {
            uint8_t resp;
            socket.receive(asio::buffer(&resp, sizeof(resp)));
        } else {
            R resp;
            socket.receive(asio::buffer(&resp, sizeof(resp)));
            return resp;
        }
    }

protected:
    asio::io_service io_ctx;
    asio::ip::tcp::socket socket;
}; // class client

} // namespace hrpc

#endif // __HRPC_CLIENT_H__
