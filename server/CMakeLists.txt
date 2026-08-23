# Server-side modules (one static library each) and executables.

add_subdirectory(modules/config)
add_subdirectory(modules/db)
add_subdirectory(modules/auth)
add_subdirectory(modules/http)

add_subdirectory(app)
add_subdirectory(migrate)

# Cross-module integration tests (label: integration).
add_subdirectory(tests/integration)
