# modulo_server_db — database access module: migration engine and the
# libpqxx connection pool shared by every repository. Qt-free by design.
# libpqxx is PUBLIC because the pool hands out pqxx::connection leases.

modulo_add_library(
    modulo_server_db
    SOURCES src/connection_pool.cpp src/migrator.cpp
    PUBLIC_DEPS libpqxx::pqxx)
