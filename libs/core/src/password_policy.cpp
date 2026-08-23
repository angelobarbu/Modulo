#include <modulo/core/password_policy.h>

namespace modulo::core {

VoidResult validatePassword(QStringView password) {
    if (password.size() < kMinPasswordLength) {
        return makeError(QStringLiteral("password.too_short"),
                         QStringLiteral("password must be at least %1 characters").arg(kMinPasswordLength));
    }
    if (password.size() > kMaxPasswordLength) {
        return makeError(QStringLiteral("password.too_long"),
                         QStringLiteral("password must be at most %1 characters").arg(kMaxPasswordLength));
    }
    return {};
}

} // namespace modulo::core
