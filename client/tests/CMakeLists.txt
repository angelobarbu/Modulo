# QML / Qt Quick tests: every tst_*.qml file in ./qml runs under the QUICK_TEST_MAIN
# runner, offscreen (no display needed).

modulo_add_qml_test(
    modulo_client_qml_tests
    QML_DIR ${CMAKE_CURRENT_SOURCE_DIR}/qml
    SOURCES qml/main.cpp)
