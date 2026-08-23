modulo_add_test(
    modulo_server_auth_password_hasher_tests
    LABEL unit
    SOURCES test_password_hasher.cpp
    DEPS modulo_server_auth)

modulo_add_test(
    modulo_server_auth_token_tests
    LABEL unit
    SOURCES test_token.cpp
    DEPS modulo_server_auth)

modulo_add_test(
    modulo_server_auth_roles_tests
    LABEL unit
    SOURCES test_roles.cpp
    DEPS modulo_server_auth)
