#pragma once

#include <QString>

namespace modulo::core {

/// Semantic version of the Modulo project, e.g. "0.1.0". Single source of
/// truth is the root CMake project() call (injected at compile time).
[[nodiscard]] QString version();

} // namespace modulo::core
