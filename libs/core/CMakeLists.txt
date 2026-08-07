# modulo_core — domain foundations shared by every other target.
# Links QtCore (QString is the project-wide string type); the deliberately
# Qt-free zone is server/modules/db + modulo_migrate, which do not use core.

modulo_add_library(
    modulo_core
    SOURCES src/version.cpp
    PUBLIC_DEPS Qt6::Core)
