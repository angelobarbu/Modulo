#include <modulo/server/db/connection_pool.h>

#include <stdexcept>
#include <utility>

namespace modulo::server::db {

// --- Lease -------------------------------------------------------------------

ConnectionPool::Lease::Lease(ConnectionPool& pool, std::unique_ptr<pqxx::connection> connection)
    : pool_{&pool}, connection_{std::move(connection)} {
}

ConnectionPool::Lease::Lease(Lease&& other) noexcept
    : pool_{std::exchange(other.pool_, nullptr)}, connection_{std::move(other.connection_)} {
}

ConnectionPool::Lease& ConnectionPool::Lease::operator=(Lease&& other) noexcept {
    if (this != &other) {
        if (pool_ != nullptr && connection_) {
            pool_->release(std::move(connection_));
        }
        pool_ = std::exchange(other.pool_, nullptr);
        connection_ = std::move(other.connection_);
    }
    return *this;
}

ConnectionPool::Lease::~Lease() {
    if (pool_ != nullptr && connection_) {
        pool_->release(std::move(connection_));
    }
}

// --- ConnectionPool ----------------------------------------------------------

ConnectionPool::ConnectionPool(std::string databaseUrl, std::size_t capacity)
    : databaseUrl_{std::move(databaseUrl)}, capacity_{capacity} {
    if (capacity_ == 0) {
        throw std::invalid_argument("ConnectionPool capacity must be at least 1");
    }
}

ConnectionPool::Lease ConnectionPool::acquire() {
    std::unique_lock lock{mutex_};
    for (;;) {
        if (!idle_.empty()) {
            auto connection = std::move(idle_.back());
            idle_.pop_back();
            return Lease{*this, std::move(connection)};
        }

        if (open_ < capacity_) {
            // Reserve the slot before connecting so concurrent callers cannot
            // overshoot the capacity while this connection is being opened.
            ++open_;
            lock.unlock();
            try {
                return Lease{*this, std::make_unique<pqxx::connection>(databaseUrl_)};
            } catch (...) {
                lock.lock();
                --open_;
                available_.notify_one();
                throw;
            }
        }

        available_.wait(lock);
    }
}

std::size_t ConnectionPool::openConnections() {
    const std::lock_guard lock{mutex_};
    return open_;
}

void ConnectionPool::release(std::unique_ptr<pqxx::connection> connection) {
    const std::lock_guard lock{mutex_};
    if (connection && connection->is_open()) {
        idle_.push_back(std::move(connection));
    } else {
        --open_; // broken connection: drop it, a fresh one is opened on demand
    }
    available_.notify_one();
}

} // namespace modulo::server::db
