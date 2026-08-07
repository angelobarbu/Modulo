# modulo_server_db — database access module: migration engine (and, from
# Increment 2, the libpqxx connection pool used by all repositories).

modulo_add_library(
    modulo_server_db
    SOURCES src/migrator.cpp
    PRIVATE_DEPS libpqxx::pqxx)
