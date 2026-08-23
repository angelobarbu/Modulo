# modulo_server_http — the REST API: QHttpServer wrapper, routes, auth guards,
# the JSON error envelope and security headers. Links the feature modules
# whose routes it registers.

modulo_add_library(
    modulo_server_http
    SOURCES src/auth_guard.cpp src/auth_routes.cpp src/responses.cpp src/server.cpp
    PUBLIC_DEPS modulo_api modulo_server_auth modulo_server_config Qt6::HttpServer
    PRIVATE_DEPS Qt6::Network)
