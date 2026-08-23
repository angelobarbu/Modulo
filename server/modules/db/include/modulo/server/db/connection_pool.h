#pragma once

#include <pqxx/pqxx>

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace modulo::server::db {

/// Fixed-capacity pool of libpqxx connections, safe to share between threads.
///
/// Connections are opened lazily on first use (constructing a pool never
/// touches the network) and handed out as RAII leases; acquire() blocks while
/// every connection is in use. A connection found closed when its lease ends
/// is discarded and re-created on demand. Connection failures propagate as
/// pqxx exceptions - this module is the Qt-free zone; callers translate them
/// into core::Result errors at the boundary.
class ConnectionPool {
public:
    /// Exclusive access to one pooled connection; returns it on destruction.
    /// A lease must not outlive its pool.
    class Lease {
    public:
        Lease(Lease&& other) noexcept;
        Lease& operator=(Lease&& other) noexcept;
        ~Lease();

        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;

        pqxx::connection& connection() const { return *connection_; }

        pqxx::connection* operator->() const { return connection_.get(); }

    private:
        friend class ConnectionPool;
        Lease(ConnectionPool& pool, std::unique_ptr<pqxx::connection> connection);

        ConnectionPool* pool_ = nullptr;
        std::unique_ptr<pqxx::connection> connection_;
    };

    explicit ConnectionPool(std::string databaseUrl, std::size_t capacity = 4);

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    /// Blocks until a connection is free. Throws pqxx::broken_connection (or
    /// another pqxx::failure) if a new connection cannot be opened.
    Lease acquire();

    std::size_t capacity() const { return capacity_; }

    /// Connections currently open (idle + leased). Observability for tests.
    std::size_t openConnections();

private:
    void release(std::unique_ptr<pqxx::connection> connection);

    std::string databaseUrl_;
    std::size_t capacity_;

    std::mutex mutex_;
    std::condition_variable available_;
    std::vector<std::unique_ptr<pqxx::connection>> idle_;
    std::size_t open_ = 0;
};

} // namespace modulo::server::db
