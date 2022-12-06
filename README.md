# hrpc: Minimal header-only RPC library

A very-simple C++-native header-only RPC library **greatly inspired** by [rpclib](https://github.com/rpclib/rpclib).

* Based on C++20 and asio
* Use 64-bit integers as RPC identifiers
* RPC handlers can acquire a handle to RPC server

## Caveats

* Accept only [trivial types](https://en.cppreference.com/w/cpp/language/classes#Trivial_class)
* Servers & clients must be homogeneous: hrpc is not equipped with a full-featured (de)serialization library, and it only packs RPC args/return values into structs. Different endianness, address width, etc. can cause a disaster.

## Prerequisites

```shell
sudo apt install -y g++-10 libasio-dev
```

## Use

Add `include` into your include folder list.
Then, include `hrpc/client.h` and `hrpc/server.h` as you wish.

### Server Example

```cpp
#include <iostream>
#include <cassert>
#include "hrpc/server.h"

static constexpr int ADD = 0x1;
static constexpr int SUB = 0x2;

int main(int argc, char *argv[]) {
    hrpc::server srv(8080);
    srv.bind(ADD, [](int a, int b) {
        return a + b;
    });
    srv.bind(SUB, [&](hrpc::server *self, int a, int b) {
        assert(self == &srv);
        return a - b;
    });

    srv.run();
    return 0;
}
```

### Client Example

```cpp
#include <iostream>
#include "hrpc/client.h"

static constexpr int ADD = 0x1;
static constexpr int SUB = 0x2;

int main() {
    hrpc::client client("127.0.0.1", 8080);

    auto result = client.call<int>(ADD, 2, 3);
    std::cout << "2 + 3 = " << result << std::endl;

    result = client.call<int>(SUB, 10, 7);
    std::cout << "10 - 7 = " << result << std::endl;
    
    return 0;
}
```
