#pragma once

// Internal to the auth module: the Qt <-> libpqxx boundary used by the
// repositories. Converts types at the edge and maps libpqxx failures to
// core::Error so repository methods return Result<T>, never throw.

#include <modulo/core/result.h>
#include <modulo/server/db/connection_pool.h>

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QTimeZone>

#include <pqxx/pqxx>

#include <cstddef>
#include <exception>
#include <string>
#include <type_traits>
#include <utility>

namespace modulo::server::auth::pg {

inline std::string toStd(const QString& value) {
    return value.toStdString();
}

inline QString fromStd(const std::string& value) {
    return QString::fromStdString(value);
}

inline pqxx::bytes toBytes(const QByteArray& value) {
    const auto* begin = reinterpret_cast<const std::byte*>(value.constData());
    return pqxx::bytes(begin, begin + value.size());
}

/// Timestamps travel as ISO-8601 UTC text into SQL and as epoch milliseconds
/// out of it (see the `epochMs` SQL helper), avoiding PostgreSQL's
/// locale-dependent timestamp text format.
inline std::string toIso(const QDateTime& value) {
    return value.toUTC().toString(Qt::ISODateWithMs).toStdString();
}

inline QDateTime fromEpochMs(long long milliseconds) {
    return QDateTime::fromMSecsSinceEpoch(milliseconds, QTimeZone::UTC);
}

/// SQL fragment turning a timestamptz column into epoch milliseconds.
inline std::string epochMs(const char* column) {
    return std::string{"(extract(epoch from "} + column + ") * 1000)::bigint";
}

/// Runs `fn(pqxx::work&)` inside one transaction on a pooled connection and
/// commits; any libpqxx failure becomes an Error with a stable "db.*" code.
template <typename Fn>
auto withTransaction(db::ConnectionPool& pool, Fn&& fn) -> core::Result<std::invoke_result_t<Fn, pqxx::work&>> {
    using Value = std::invoke_result_t<Fn, pqxx::work&>;
    try {
        auto lease = pool.acquire();
        pqxx::work tx{lease.connection()};
        if constexpr (std::is_void_v<Value>) {
            std::forward<Fn>(fn)(tx);
            tx.commit();
            return {};
        } else {
            Value value = std::forward<Fn>(fn)(tx);
            tx.commit();
            return value;
        }
    } catch (const pqxx::unique_violation& error) {
        return core::makeError(QStringLiteral("db.unique_violation"), fromStd(error.what()));
    } catch (const pqxx::sql_error& error) {
        return core::makeError(QStringLiteral("db.query_failed"), fromStd(error.what()));
    } catch (const pqxx::broken_connection& error) {
        return core::makeError(QStringLiteral("db.unavailable"), fromStd(error.what()));
    } catch (const std::exception& error) {
        return core::makeError(QStringLiteral("db.error"), fromStd(error.what()));
    }
}

} // namespace modulo::server::auth::pg
