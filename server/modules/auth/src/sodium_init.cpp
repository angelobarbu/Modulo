#include "sodium_init.h"

#include <sodium.h>

#include <mutex>
#include <stdexcept>

namespace modulo::server::auth {

void ensureSodium() {
    static std::once_flag once;
    std::call_once(once, [] {
        if (sodium_init() < 0) {
            throw std::runtime_error("libsodium failed to initialise");
        }
    });
}

} // namespace modulo::server::auth
