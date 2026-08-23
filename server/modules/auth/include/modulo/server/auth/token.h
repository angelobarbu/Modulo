#pragma once

#include <QByteArray>
#include <QString>

namespace modulo::server::auth::token {

inline constexpr int kTokenBytes = 32;
inline constexpr int kDigestBytes = 32;

/// A new opaque session token: 32 CSPRNG bytes (libsodium randombytes_buf -
/// an explicit cryptographic guarantee, which QRandomGenerator does not make)
/// encoded as base64url without padding (43 characters). Returned to the
/// client exactly once and never stored.
QString generate();

/// SHA-256 digest (32 bytes, QCryptographicHash) of a token as presented by
/// the client - the only form ever stored or compared. Any string is
/// accepted; a malformed token simply never matches a session row.
QByteArray digest(const QString& token);

} // namespace modulo::server::auth::token
