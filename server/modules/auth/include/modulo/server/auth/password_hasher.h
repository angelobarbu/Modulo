#pragma once

#include <modulo/core/result.h>

#include <QString>

namespace modulo::server::auth {

/// Argon2id password hashing via libsodium (crypto_pwhash_str).
///
/// Hashes are self-describing strings (algorithm, parameters, salt, digest)
/// and are stored verbatim in users.password_hash. Cost parameters are
/// libsodium's INTERACTIVE profile (64 MiB, 2 passes): above the OWASP
/// minimum for Argon2id and fast enough for a login round-trip.
class PasswordHasher {
public:
    /// Error code "auth.hash_failed" only if libsodium cannot allocate.
    static core::Result<QString> hash(const QString& password);

    /// Constant-time verification; false for a malformed hash.
    static bool verify(const QString& hash, const QString& password);

    /// True when the stored hash used weaker parameters than the current
    /// profile - callers may re-hash after a successful verify().
    static bool needsRehash(const QString& hash);
};

} // namespace modulo::server::auth
