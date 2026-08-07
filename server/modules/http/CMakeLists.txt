# modulo_server_http — the REST API server: owns the QHttpServer, registers
# every route, and enforces the uniform JSON error envelope.

modulo_add_library(
    modulo_server_http
    SOURCES src/server.cpp
    PUBLIC_DEPS modulo_api modulo_server_config Qt6::HttpServer
    PRIVATE_DEPS Qt6::Network)
