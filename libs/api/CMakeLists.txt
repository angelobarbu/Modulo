# modulo_api — request/response DTOs and their JSON mappings, shared verbatim
# by the server and every client so both sides agree on the wire format.

modulo_add_library(
    modulo_api
    SOURCES src/auth.cpp src/error.cpp src/health.cpp src/json.cpp
    PUBLIC_DEPS modulo_core Qt6::Core)
