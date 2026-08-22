#include <modulo/core/version.h>

namespace modulo::core {

QString version() {
    return QStringLiteral(MODULO_VERSION);
}

} // namespace modulo::core
