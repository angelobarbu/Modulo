# modulo_migrate — command-line migration runner (scripts/migrate.sh wraps it).

modulo_add_executable(
    modulo_migrate
    SOURCES main.cpp
    DEPS modulo_server_db)
