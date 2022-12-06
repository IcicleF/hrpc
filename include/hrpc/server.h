#pragma once

#ifndef __HRPC_SERVER_H__
#define __HRPC_SERVER_H__

#include <atomic>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <asio.hpp>

#include "common.h"
#include "detail/call.h"
#include "detail/func_traits.h"
#include "detail/remove_first_arg.h"
#include "detail/serdes.h"

namespace hrpc {

class server {
public:
    server(uint16_t port) : port(port), acceptor(io_ctx), should_stop(false) {}
    ~server() {
        should_stop = true;

        asio::error_code err;
        acceptor.close(err);
        io_ctx.stop();
    }

    template <typename F> void bind(hrpc_id_t id, F func) {
        if (handlers.find(id) != handlers.end()) {
            throw std::logic_error("duplicate handler id " +
                                   std::to_string(id));
        }
        bind(id, func, typename detail::func_kind_info<F>::result_kind{},
             typename detail::func_kind_info<F>::args_kind{});
    }

    void run() {
        if (should_stop) {
            throw std::logic_error("server already stopped, cannot restart");
        }

        asio::ip::tcp::endpoint ep = {asio::ip::tcp::v4(), port};
        acceptor.open(ep.protocol());
        acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(ep);
        acceptor.listen();
        start_accept();
        while (!should_stop) {
            io_ctx.run_one();
        }
    }

    void stop() { should_stop = true; }

protected:
    void start_accept() {
        connections.emplace_back(asio::ip::tcp::socket(io_ctx), 0);
        auto index = connections.size() - 1;
        acceptor.async_accept(connections.back().socket,
                              [this, index](const asio::error_code &err) {
                                  if (!err) {
                                      start_receive(index);
                                  }
                                  start_accept();
                              });
    }

    void start_receive(size_t index) {
        asio::async_read(
            connections[index].socket,
            asio::buffer(&connections[index].id, sizeof(hrpc_id_t)),
            asio::transfer_all(),
            std::bind(&server::handle_incoming_rpc, this, index,
                      std::placeholders::_1, std::placeholders::_2));
    }

    void handle_incoming_rpc(size_t index, const asio::error_code &err,
                             size_t nbytes) {
        if (err) {
            return;
        }
        if (nbytes == sizeof(hrpc_id_t)) {
            hrpc_id_t id = connections[index].id;
            auto it = handlers.find(id);
            if (it != handlers.end()) {
                // Retrieve request
                size_t req_size = it->second.req_size;
                uint8_t *req_buf = new uint8_t[req_size];
                connections[index].socket.receive(
                    asio::buffer(req_buf, req_size));

                // Get & send response
                size_t resp_size = it->second.resp_size;
                uint8_t *resp_buf = new uint8_t[resp_size];
                it->second.adaptor(req_buf, resp_buf);
                connections[index].socket.send(
                    asio::buffer(resp_buf, resp_size));
            }
        }
        start_receive(index);
    }

    // https://github.com/rpclib/rpclib
    //
    // Copyright (c) 2015-2017, Tamás Szelei
    //
    // Permission is hereby granted, free of charge, to any person obtaining a
    // copy of this software and associated documentation files (the
    // "Software"), to deal in the Software without restriction, including
    // without limitation the rights to use, copy, modify, merge, publish,
    // distribute, sublicense, and/or sell copies of the Software, and to permit
    // persons to whom the Software is furnished to do so, subject to the
    // following conditions:
    //
    // The above copyright notice and this permission notice shall be included
    // in all copies or substantial portions of the Software.
    //
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    // OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    // MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
    // NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    // DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
    // OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
    // USE OR OTHER DEALINGS IN THE SOFTWARE.
    //
    template <typename F>
    void bind(hrpc_id_t id, F func, detail::tags::void_result const &,
              detail::tags::zero_arg const &) {
        handlers[id] = {0, 1,
                        [func](uint8_t const *req_buf, uint8_t *resp_buf) {
                            (void)req_buf;
                            func();
                            resp_buf[0] = 0;
                        }};
    }

    template <typename F>
    void bind(hrpc_id_t id, F func, detail::tags::void_result const &,
              detail::tags::nonzero_arg const &) {
        using args_type = typename detail::func_traits<F>::args_type;
        if constexpr (std::is_same_v<
                          typename std::tuple_element<0, args_type>::type,
                          server *>) {
            using real_args_type =
                typename detail::remove_first_arg<args_type>::type;
            handlers[id] = {
                sizeof(args_type), 1,
                [this, func](uint8_t const *req_buf, uint8_t *resp_buf) {
                    auto req_args =
                        detail::deserialize<real_args_type>(req_buf);
                    auto req_args_with_self =
                        std::tuple_cat(std::make_tuple(this), req_args);
                    detail::call(func, req_args_with_self);
                    resp_buf[0] = 0;
                }};
        } else {
            handlers[id] = {sizeof(args_type), 1,
                            [func](uint8_t const *req_buf, uint8_t *resp_buf) {
                                args_type req_args =
                                    detail::deserialize<args_type>(req_buf);
                                detail::call(func, req_args);
                                resp_buf[0] = 0;
                            }};
        }
    }

    template <typename F>
    void bind(hrpc_id_t id, F func, detail::tags::nonvoid_result const &,
              detail::tags::zero_arg const &) {
        using result_type = typename detail::func_traits<F>::result_type;
        static_assert(std::is_trivial_v<result_type>,
                      "handler return type must be trivial");
        handlers[id] = {0, sizeof(result_type),
                        [func](uint8_t const *req_buf, uint8_t *resp_buf) {
                            (void)req_buf;
                            result_type resp = func();
                            *reinterpret_cast<result_type *>(resp_buf) = resp;
                        }};
    }

    template <typename F>
    void bind(hrpc_id_t id, F func, detail::tags::nonvoid_result const &,
              detail::tags::nonzero_arg const &) {
        using args_type = typename detail::func_traits<F>::args_type;
        using result_type = typename detail::func_traits<F>::result_type;
        static_assert(std::is_trivial_v<result_type>,
                      "handler return type must be trivial");

        if constexpr (std::is_same_v<std::tuple_element_t<0, args_type>,
                                     server *>) {
            using real_args_type =
                typename detail::remove_first_arg<args_type>::type;
            handlers[id] = {
                sizeof(args_type), sizeof(result_type),
                [this, func](uint8_t const *req_buf, uint8_t *resp_buf) {
                    auto req_args =
                        detail::deserialize<real_args_type>(req_buf);
                    auto req_args_with_self =
                        std::tuple_cat(std::make_tuple(this), req_args);
                    result_type resp = detail::call(func, req_args_with_self);
                    *reinterpret_cast<result_type *>(resp_buf) = resp;
                }};
        } else {
            handlers[id] = {sizeof(args_type), sizeof(result_type),
                            [func](uint8_t const *req_buf, uint8_t *resp_buf) {
                                args_type req_args =
                                    detail::deserialize<args_type>(req_buf);
                                result_type resp = detail::call(func, req_args);
                                *reinterpret_cast<result_type *>(resp_buf) =
                                    resp;
                            }};
        }
    }

    uint16_t port;
    asio::io_service io_ctx;
    asio::ip::tcp::acceptor acceptor;
    std::atomic<bool> should_stop;

    struct identified_connection {
        asio::ip::tcp::socket socket;
        hrpc_id_t id;
    };
    std::vector<identified_connection> connections;

    using adaptor_type = std::function<void(uint8_t *, uint8_t *)>;
    struct sized_handler {
        size_t req_size;
        size_t resp_size;
        adaptor_type adaptor;
    };
    std::unordered_map<hrpc_id_t, sized_handler> handlers;
}; // class server

} // namespace hrpc

#endif // __HRPC_SERVER_H__
