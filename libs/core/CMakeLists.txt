# modulo_core — domain foundations shared by every other target.

modulo_add_library(
    modulo_core
    SOURCES src/version.cpp
    PUBLIC_DEPS Qt6::Core)
