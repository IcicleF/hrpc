#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <hrpc/client.h>
#include <hrpc/server.h>

static constexpr uint16_t port = 8392;
static constexpr hrpc::hrpc_id_t FOO = 0x0;
static constexpr hrpc::hrpc_id_t ADD = 0x1;

void foo() { std::cout << "foo was called!" << std::endl; }

int main(int argc, char **argv) {
    if (argc > 1) {
        // Client 1
        hrpc::client client(argv[1], port);
        std::cout << "client1 built" << std::endl;

        auto ret = client.call<int>(ADD, 1, 2);
        std::cout << ret << std::endl;

        ret = client.call<int>(ADD, 1, 3);
        std::cout << ret << std::endl;

        // Client 2
        hrpc::client client2(argv[1], port);
        std::cout << "client2 built" << std::endl;

        ret = client2.call<int>(ADD, 2, 4);
        std::cout << ret << std::endl;

        client2.call<void>(FOO);
    } else {
        hrpc::server server(port);
        std::cout << "server built" << std::endl;

        server.bind(FOO, &foo);
        server.bind(ADD, [](int a, int b) { return a + b; });
        std::cout << "handler bound" << std::endl;

        server.run();
    }
    return 0;
}
