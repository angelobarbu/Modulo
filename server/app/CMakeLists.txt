# modulo_server — the REST API server executable.

modulo_add_executable(
    modulo_server
    SOURCES main.cpp
    DEPS modulo_server_http Qt6::Core)
