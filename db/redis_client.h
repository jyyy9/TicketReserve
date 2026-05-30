#pragma once
#include <string>
#include <hiredis/hiredis.h>

class Redis
{
public:
    static bool set(const std::string &key, const std::string &val, int expire = 300);
    static bool get(const std::string &key, std::string &val);
    static bool del(const std::string &key);
    static bool lock(const std::string &key, const std::string &val, int expire = 5);
    static bool unlock(const std::string &key, const std::string &val);
};