#include "sodium_init.h"

#include <modulo/server/auth/password_hasher.h>

#include <sodium.h>

#include <array>

namespace modulo::server::auth {

namespace {

constexpr unsigned long long kOpsLimit = crypto_pwhash_OPSLIMIT_INTERACTIVE;
constexpr std::size_t kMemLimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;

} // namespace

core::Result<QString> PasswordHasher::hash(const QString& password) {
    ensureSodium();

    const QByteArray utf8 = password.toUtf8();
    std::array<char, crypto_pwhash_STRBYTES> encoded{};
    if (crypto_pwhash_str(encoded.data(), utf8.constData(), static_cast<unsigned long long>(utf8.size()), kOpsLimit,
                          kMemLimit) != 0) {
        return core::makeError(QStringLiteral("auth.hash_failed"),
                               QStringLiteral("password hashing failed (out of memory)"));
    }
    return QString::fromLatin1(encoded.data()); // NUL-terminated ASCII
}

bool PasswordHasher::verify(const QString& hash, const QString& password) {
    ensureSodium();

    const QByteArray encoded = hash.toLatin1();
    if (encoded.size() >= static_cast<qsizetype>(crypto_pwhash_STRBYTES)) {
        return false; // cannot be a valid crypto_pwhash_str output
    }
    std::array<char, crypto_pwhash_STRBYTES> buffer{};
    std::copy(encoded.cbegin(), encoded.cend(), buffer.begin());

    const QByteArray utf8 = password.toUtf8();
    return crypto_pwhash_str_verify(buffer.data(), utf8.constData(), static_cast<unsigned long long>(utf8.size())) == 0;
}

bool PasswordHasher::needsRehash(const QString& hash) {
    ensureSodium();

    const QByteArray encoded = hash.toLatin1();
    if (encoded.size() >= static_cast<qsizetype>(crypto_pwhash_STRBYTES)) {
        return true;
    }
    std::array<char, crypto_pwhash_STRBYTES> buffer{};
    std::copy(encoded.cbegin(), encoded.cend(), buffer.begin());

    // 0 = parameters match; 1 = weaker than current; -1 = unparsable (treat as rehash).
    return crypto_pwhash_str_needs_rehash(buffer.data(), kOpsLimit, kMemLimit) != 0;
}

} // namespace modulo::server::auth
