// modulo_migrate — command-line migration runner.
//
// Usage:
//   modulo_migrate [--url <postgres-url>] [--dir <migrations-dir>]
//
// The database URL falls back to the MODULO_DB_URL environment variable;
// the migrations directory defaults to db/migrations relative to the
// current working directory (scripts/migrate.sh passes it explicitly).

#include <modulo/server/db/migrator.h>

#include <cstdlib>
#include <iostream>
#include <print>
#include <span>
#include <string>

namespace {

struct Options {
    std::string url;
    std::string dir = "db/migrations";
    bool help = false;
};

Options parseArguments(std::span<char*> args) {
    Options options;
    if (const char* env = std::getenv("MODULO_DB_URL")) {
        options.url = env;
    }

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg{args[i]};
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--url" && i + 1 < args.size()) {
            options.url = args[++i];
        } else if (arg == "--dir" && i + 1 < args.size()) {
            options.dir = args[++i];
        } else {
            throw std::invalid_argument(std::string{"unknown or incomplete argument: "} + std::string{arg});
        }
    }
    return options;
}

} // namespace

int main(int argc, char* argv[]) {
    Options options;
    try {
        options = parseArguments(std::span{argv, static_cast<std::size_t>(argc)});
    } catch (const std::invalid_argument& error) {
        std::println(stderr, "error: {}", error.what());
        return EXIT_FAILURE;
    }

    if (options.help) {
        std::println("usage: modulo_migrate [--url <postgres-url>] [--dir <migrations-dir>]");
        std::println("       --url defaults to $MODULO_DB_URL; --dir defaults to db/migrations");
        return EXIT_SUCCESS;
    }

    if (options.url.empty()) {
        std::println(stderr, "error: no database URL (pass --url or set MODULO_DB_URL)");
        return EXIT_FAILURE;
    }

    try {
        modulo::server::db::Migrator migrator{options.url, options.dir,
                                              [](std::string_view line) { std::println("{}", line); }};
        const auto result = migrator.run();
        std::println("migrations: {} applied, {} skipped", result.applied, result.skipped);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        // Never echo options.url here — it may contain credentials.
        std::println(stderr, "error: {}", error.what());
        return EXIT_FAILURE;
    }
}
