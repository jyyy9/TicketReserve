#pragma once
#include <hiredis/hiredis.h>
#include <vector>
#include <mutex>
#include <string>

// Redis 连接池
class RedisPool {
public:
    static RedisPool& instance();
    bool init(const std::string& ip, int port, int size = 10);
    redisContext* get();
    void put(redisContext* ctx);

private:
    RedisPool() = default;
    redisContext* create_conn(const std::string& ip, int port);

    std::vector<redisContext*> _conns;
    std::mutex _mtx;
};

// RAII 自动归还连接
class RedisGuard {
public:
    RedisGuard();
    ~RedisGuard();
    redisContext* ctx();

private:
    redisContext* _ctx;
};