# modulo_server_db — database access module and migration engine.

modulo_add_library(
    modulo_server_db
    SOURCES src/migrator.cpp
    PRIVATE_DEPS libpqxx::pqxx)
