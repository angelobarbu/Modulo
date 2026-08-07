#pragma once

#include <QString>

#include <expected>
#include <utility>

namespace modulo::core {

/// Error value carried by Result.
///
/// `code` is a stable, machine-readable identifier in dotted-snake form
/// (e.g. "config.invalid_port", "http.bind_failed") — it is what tests and
/// API clients match on. `message` is human-readable detail and carries no
/// stability guarantee.
struct Error {
    QString code;
    QString message;
};

/// Project-wide result type: a value of T, or an Error.
///
/// Used instead of exceptions on expected failure paths (bad input,
/// unavailable resources). Exceptions remain for genuinely exceptional,
/// non-recoverable situations.
template <typename T>
using Result = std::expected<T, Error>;

/// Result for operations that produce no value on success.
using VoidResult = std::expected<void, Error>;

/// Convenience factory: `return makeError("config.invalid_port", "...");`
/// converts implicitly to any Result<T>.
[[nodiscard]] inline std::unexpected<Error> makeError(QString code, QString message) {
    return std::unexpected{Error{std::move(code), std::move(message)}};
}

} // namespace modulo::core
