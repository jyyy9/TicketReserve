#include "redis_client.h"
#include "redis_pool.h"
#include <hiredis/hiredis.h>

using namespace std;

bool Redis::set(const string& key, const string& val, int expire) {
    RedisGuard guard;
    if (!guard.ctx()) return false;
    redisReply* r = (redisReply*)redisCommand(guard.ctx(), "SETEX %s %d %s", key.c_str(), expire, val.c_str());
    bool ok = r && r->type == REDIS_REPLY_STATUS;
    if (r) freeReplyObject(r);
    return ok;
}

bool Redis::get(const string& key, string& val) {
    RedisGuard guard;
    if (!guard.ctx()) return false;
    redisReply* r = (redisReply*)redisCommand(guard.ctx(), "GET %s", key.c_str());
    if (!r || r->type != REDIS_REPLY_STRING) {
        if (r) freeReplyObject(r);
        return false;
    }
    val = r->str;
    freeReplyObject(r);
    return true;
}

bool Redis::del(const string& key) {
    RedisGuard guard;
    if (!guard.ctx()) return false;
    redisReply* r = (redisReply*)redisCommand(guard.ctx(), "DEL %s", key.c_str());
    bool ok = r && r->type == REDIS_REPLY_INTEGER;
    if (r) freeReplyObject(r);
    return ok;
}

bool Redis::lock(const string& key, const string& val, int expire) {
    RedisGuard guard;
    if (!guard.ctx()) return false;
    redisReply* r = (redisReply*)redisCommand(guard.ctx(), "SET %s %s NX EX %d", key.c_str(), val.c_str(), expire);
    bool ok = r && r->type == REDIS_REPLY_STRING;
    if (r) freeReplyObject(r);
    return ok;
}

bool Redis::unlock(const string& key, const string& val) {
    RedisGuard guard;
    if (!guard.ctx()) return false;
    const char* lua = "if redis.call('get',KEYS[1])==ARGV[1] then return redis.call('del',KEYS[1]) else return 0 end";
    redisReply* r = (redisReply*)redisCommand(guard.ctx(), "EVAL %s 1 %s %s", lua, key.c_str(), val.c_str());
    bool ok = r && r->integer == 1;
    if (r) freeReplyObject(r);
    return ok;
}