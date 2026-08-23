# Cross-module integration tests: real QHttpServer in-process, real HTTP
# client, and the real dockerized test database (schema from db/migrations).
# Opt-in via MODULO_TEST_DB_URL — see tests/support/include/modulo/testing/integration.h.

modulo_add_test(
    modulo_integration_tests
    LABEL integration
    SOURCES test_health_endpoint.cpp
    DEPS modulo_server_http Qt6::Network)

modulo_add_test(
    modulo_auth_repositories_tests
    LABEL integration
    SOURCES test_auth_repositories.cpp
    DEPS modulo_server_auth Qt6::Network)

modulo_add_test(
    modulo_auth_flow_tests
    LABEL integration
    SOURCES test_auth_flow.cpp
    DEPS modulo_server_http Qt6::Network)
