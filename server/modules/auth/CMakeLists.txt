# modulo_server_auth — authentication & RBAC: Argon2id password hashing,
# opaque session tokens, and the user/session repositories.

modulo_add_library(
    modulo_server_auth
    SOURCES src/password_hasher.cpp
            src/roles.cpp
            src/session_repository.cpp
            src/sodium_init.cpp
            src/token.cpp
            src/user_repository.cpp
    PUBLIC_DEPS modulo_core modulo_server_db Qt6::Core
    PRIVATE_DEPS sodium::sodium)
