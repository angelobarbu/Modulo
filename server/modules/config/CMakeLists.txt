# modulo_server_config — server process configuration from the environment.

modulo_add_library(
    modulo_server_config
    SOURCES src/config.cpp
    PUBLIC_DEPS modulo_core Qt6::Core)
