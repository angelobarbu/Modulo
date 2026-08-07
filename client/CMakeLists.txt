# modulo_client — the QML desktop application.

modulo_add_qml_app(
    modulo_client
    URI Modulo
    SOURCES include/modulo/client/api_client.h src/api_client.cpp src/main.cpp
    QML_FILES qml/Main.qml
    DEPS modulo_api Qt6::Quick Qt6::QuickControls2)
