modulo_add_test(
    modulo_core_tests
    LABEL unit
    SOURCES test_version.cpp
    DEPS modulo_core)

modulo_add_test(
    modulo_core_password_policy_tests
    LABEL unit
    SOURCES test_password_policy.cpp
    DEPS modulo_core)
