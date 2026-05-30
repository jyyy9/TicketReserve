#include "mysql_pool.h"

#include <iostream>

using namespace std;

MySQLPool& MySQLPool::instance()
{
    static MySQLPool pool;
    return pool;
}

bool MySQLPool::init(const string& host,
                     const string& user,
                     const string& passwd,
                     const string& dbname,
                     int port,
                     int size)
{
    unique_lock<mutex> lock(_mtx);

    if (!_pool.empty())
    {
        return true;
    }

    _host = host;
    _user = user;
    _passwd = passwd;
    _dbname = dbname;
    _port = port;
    _size = size;

    for (int i = 0; i < _size; ++i)
    {
        MYSQL* conn = create_conn();
        if (!conn)
        {
            cerr << "MySQL connect failed" << endl;
            return false;
        }
        _pool.push_back(conn);
    }

    cout << "MySQL pool init success, size: " << _size << endl;
    return true;
}

MYSQL* MySQLPool::create_conn()
{
    MYSQL* conn = mysql_init(nullptr);
    if (!conn)
    {
        return nullptr;
    }

    int timeout = 3;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    if (!mysql_real_connect(conn, _host.c_str(), _user.c_str(), _passwd.c_str(),
                           _dbname.c_str(), _port, nullptr, 0))
    {
        mysql_close(conn);
        return nullptr;
    }

    mysql_query(conn, "set names utf8mb4");
    return conn;
}

MYSQL* MySQLPool::get()
{
    unique_lock<mutex> lock(_mtx);

    if (_pool.empty())
    {
        return nullptr;
    }

    MYSQL* conn = _pool.back();
    _pool.pop_back();

    if (mysql_ping(conn) != 0)
    {
        mysql_close(conn);
        conn = create_conn();
    }

    return conn;
}

void MySQLPool::put(MYSQL* conn)
{
    if (!conn)
    {
        return;
    }

    unique_lock<mutex> lock(_mtx);
    _pool.push_back(conn);
}

void MySQLPool::close_all()
{
    unique_lock<mutex> lock(_mtx);

    for (MYSQL* conn : _pool)
    {
        mysql_close(conn);
    }
    _pool.clear();
}
