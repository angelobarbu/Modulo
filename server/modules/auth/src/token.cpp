#include "sodium_init.h"

#include <modulo/server/auth/token.h>

#include <QCryptographicHash>

#include <sodium.h>

namespace modulo::server::auth::token {

QString generate() {
    ensureSodium();

    QByteArray random{kTokenBytes, Qt::Uninitialized};
    randombytes_buf(random.data(), static_cast<std::size_t>(random.size()));
    return QString::fromLatin1(random.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray digest(const QString& token) {
    return QCryptographicHash::hash(token.toUtf8(), QCryptographicHash::Sha256);
}

} // namespace modulo::server::auth::token
