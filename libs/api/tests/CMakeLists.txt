modulo_add_test(
    modulo_api_health_dto_tests
    LABEL unit
    SOURCES test_health_dto.cpp
    DEPS modulo_api)

modulo_add_test(
    modulo_api_error_dto_tests
    LABEL unit
    SOURCES test_error_dto.cpp
    DEPS modulo_api)

modulo_add_test(
    modulo_api_json_tests
    LABEL unit
    SOURCES test_json.cpp
    DEPS modulo_api)
