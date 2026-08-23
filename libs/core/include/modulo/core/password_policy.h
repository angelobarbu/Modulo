#pragma once

#include <modulo/core/result.h>

#include <QStringView>

namespace modulo::core {

/// Password rules shared by the server (registration) and the client (form
/// validation) so both sides agree before a request is ever sent.
inline constexpr qsizetype kMinPasswordLength = 10;
inline constexpr qsizetype kMaxPasswordLength = 128;

/// Error codes: "password.too_short", "password.too_long".
VoidResult validatePassword(QStringView password);

} // namespace modulo::core
