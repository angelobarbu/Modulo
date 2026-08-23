#pragma once

// Internal to the auth module: libsodium must be initialised exactly once
// before any of its functions is used.

namespace modulo::server::auth {

/// Idempotent, thread-safe. Throws std::runtime_error if libsodium cannot
/// initialise (a broken install - genuinely exceptional, not a Result path).
void ensureSodium();

} // namespace modulo::server::auth
