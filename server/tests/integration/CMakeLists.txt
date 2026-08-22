# Cross-module integration tests: real QHttpServer in-process, real HTTP
# client, and (from Increment 2) the real dockerized test database.
# Opt-in via MODULO_TEST_DB_URL — see tests/support/include/modulo/testing/integration.h.

modulo_add_test(
    modulo_integration_tests
    LABEL integration
    SOURCES test_health_endpoint.cpp
    DEPS modulo_server_http Qt6::Network)
