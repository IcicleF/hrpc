#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <hrpc/client.h>
#include <hrpc/server.h>

static constexpr uint16_t port = 8392;
static constexpr hrpc::hrpc_id_t FOO = 0x0;

void foo(hrpc::server *server) {
    std::cout << "foo was called!" << std::endl;
    server->stop();
}

int main(int argc, char **argv) {
    if (argc > 1) {
        // Client 1
        hrpc::client client(argv[1], port);
        std::cout << "client built" << std::endl;

        client.call<void>(FOO);
    } else {
        hrpc::server server(port);
        std::cout << "server built" << std::endl;

        server.bind(FOO, &foo);
        std::cout << "handler bound" << std::endl;

        server.run();
        std::cout << "server stopped" << std::endl;
    }
    return 0;
}
