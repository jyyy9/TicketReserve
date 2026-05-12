#include "redis_pool.h"
#include <iostream>
using namespace std;

// ====================== RedisPool 实现 ======================
RedisPool& RedisPool::instance() {
    static RedisPool pool;
    return pool;
}

bool RedisPool::init(const string& ip, int port, int size) {
    unique_lock<mutex> lock(_mtx);
    if (!_conns.empty()) return true;

    for (int i = 0; i < size; ++i) {
        redisContext* ctx = create_conn(ip, port);
        if (!ctx) {
            cerr << "Redis 连接创建失败" << endl;
            return false;
        }
        _conns.push_back(ctx);
    }
    cout << "Redis 连接池初始化成功，连接数：" << size << endl;
    return true;
}

redisContext* RedisPool::get() {
    unique_lock<mutex> lock(_mtx);
    if (_conns.empty()) return nullptr;

    redisContext* ctx = _conns.back();
    _conns.pop_back();

    // 心跳检测
    redisReply* reply = (redisReply*)redisCommand(ctx, "PING");
    bool ok = (reply && reply->type == REDIS_REPLY_STATUS && string(reply->str) == "PONG");
    if (reply) freeReplyObject(reply);

    if (!ok) {
        redisFree(ctx);
        ctx = create_conn("127.0.0.1", 6379);
    }
    return ctx;
}

void RedisPool::put(redisContext* ctx) {
    if (!ctx) return;
    unique_lock<mutex> lock(_mtx);
    _conns.push_back(ctx);
}

redisContext* RedisPool::create_conn(const string& ip, int port) {
    struct timeval tv = {1, 0};
    redisContext* ctx = redisConnectWithTimeout(ip.c_str(), port, tv);
    if (!ctx || ctx->err) {
        if (ctx) redisFree(ctx);
        return nullptr;
    }
    return ctx;
}

// ====================== RedisGuard 实现 ======================
RedisGuard::RedisGuard() {
    _ctx = RedisPool::instance().get();
}

RedisGuard::~RedisGuard() {
    RedisPool::instance().put(_ctx);
}

redisContext* RedisGuard::ctx() {
    return _ctx;
}