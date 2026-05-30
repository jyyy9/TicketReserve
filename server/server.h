#pragma once

#include <event.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>
#include <hiredis/hiredis.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

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

private:
    int c;
    struct event* c_ev;
    Json::FastWriter writer;
    Json::Value val;

    string username;
    string usertel;
    string passwd;
    string realname;
    GENDER gen;
    string id_card;
};
