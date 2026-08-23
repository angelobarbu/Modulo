# modulo_server_auth — authentication: Argon2id password hashing, opaque
# session tokens, the role catalogue, user/session repositories and the
# AuthService use cases on top of them. libsodium stays a private detail.

modulo_add_library(
    modulo_server_auth
    SOURCES src/auth_service.cpp
            src/logging.cpp
            src/password_hasher.cpp
            src/roles.cpp
            src/session_repository.cpp
            src/sodium_init.cpp
            src/token.cpp
            src/user_repository.cpp
    PUBLIC_DEPS modulo_core modulo_server_db Qt6::Core
    PRIVATE_DEPS sodium::sodium)
