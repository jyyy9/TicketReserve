#pragma once

#include <event.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>
<<<<<<< HEAD
#include <hiredis/hiredis.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
=======
#include<hiredis/hiredis.h>
#include<vector>//存放连接
#include<mutex>//多线程锁
#include <queue>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <cstring>

#include "../common/OpType.h"

#define LIS_MAX 10

using namespace std;

class socket_listen
{
public:
    socket_listen()
        : sockfd(-1)
        , m_port(6789)
        , m_ips("127.0.0.1")
        , base(nullptr)
    {
    }

    socket_listen(string ips, short port)
        : sockfd(-1)
        , m_ips(ips)
        , m_port(port)
        , base(nullptr)
    {
    }

    bool socket_init();
    int accept_client();
    bool add_client(int client_fd);

    void Set_base(struct event_base* base)
    {
        this->base = base;
    }

    int Get_sockfd() const
    {
        return sockfd;
    }

    struct event_base* Get_base() const
    {
        return base;
    }

private:
    int sockfd;
    short m_port;
    string m_ips;
    struct event_base* base;
};

class socket_con
{
public:
    explicit socket_con(int fd)
        : c(fd)
        , c_ev(nullptr)
        , gen(GENDER::ERR)
    {
    }

    ~socket_con()
    {
        if (c_ev != nullptr)
        {
            event_free(c_ev);
        }
        close(c);
    }

    void Set_ev(struct event* ev)
    {
        c_ev = ev;
    }

    void Recv_data();
    void Send_err();
    void Send_ok();

    void User_Register();
    void User_Login();
    void User_Show_Ticket();
    void User_Book_Ticket();
    void User_Show_My_Ticlet();
    void User_Cancel_Ticket();

    void AddRef() { ref_++; }
    void ReleaseRef() { if (--ref_ == 0) delete this; }

private:
    int c;
    struct event* c_ev;
    Json::FastWriter writer;
    Json::Value val;

<<<<<<< HEAD
    string username;
    string usertel;
    string passwd;
    string realname;
    GENDER gen;
    string id_card;
};
=======
    std::atomic<int> ref_{1};

    string username; // 用户名
    string usertel;  // 电话
    string passwd;   // 密码
    string realname; // 真实姓名
    GENDER gen;      // 性别：0未知 1男 2女
    string id_card;  // 身份证号
};

// 线程池
class ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool(int num = 10) {
        for (int i = 0; i < num; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) return;
                        task = move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }

    void add_task(Task task) {
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.emplace(move(task));
        cv_.notify_one();
    }

private:
    vector<std::thread> workers_;
    queue<Task> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_ = false;
};

// 全局线程池（extern）
extern ThreadPool g_pool;
>>>>>>> 997ecac1d62ed16b323db5dc5da067eade2cba44
