#pragma once

#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <jsoncpp/json/json.h>
#include "../common/OpType.h"
#include <mysql/mysql.h>
#include<hiredis/hiredis.h>
#include<vector>//存放连接
#include<mutex>//多线程锁

using namespace std;

#define LIS_MAX 10

// 第一个类：处理监听套接字（服务器端）
class socket_listen
{
public:
    socket_listen()
    {
        sockfd = -1;
        m_port = 6789;
        m_ips = "127.0.0.1";
    }
    socket_listen(string ips, short port) : m_ips(ips), m_port(port)
    {
        sockfd = -1;
    }

    bool socket_init();
    int accept_client();

    void Set_base(struct event_base *base)
    {
        this->base = base;
    }

    int Get_sockfd() const
    {
        return sockfd;
    }

    struct event_base *Get_base() const
    {
        return base;
    }

    bool add_client(int client_fd);

private:
    int sockfd;
    short m_port;
    string m_ips;
    struct event_base *base;
};

// 第二个类：处理连接套接字（客户端）
class socket_con
{
public:
    socket_con(int fd) : c(fd) // 把传进来的fd赋值给变量c
    {
        c_ev = NULL; // 事件指针初始空
    }
    void Set_ev(struct event *ev)
    {
        c_ev = ev;
    }
    ~socket_con()
    {
        event_free(c_ev);
        close(c);
    }
    void Recv_data();
    void Send_err();
    void Send_ok();

    // 业务功能
    void User_Register();    // 注册
    void User_Login();       // 登录
    void User_Show_Ticket(); // 查看预约信息
    void User_Book_Ticket(); // 预定操作
    void User_Show_My_Ticlet();//查看我的预约信息
    void User_Cancel_Ticket();//取消预约

private:
    int c;              // 文件描述符
    struct event *c_ev; // 是一个libevent事件,是一个指针
    Json::FastWriter writer;
    Json::Value val; // 存储从客户端传入的数据，Json格式

    string username; // 用户名
    string usertel;  // 电话
    string passwd;   // 密码
    string realname; // 真实姓名
    GENDER gen;      // 性别：0未知 1男 2女
    string id_card;  // 身份证号
};

// MYSQL类
class mysql_client
{
public:
    mysql_client()
    {
        db_host = "127.0.0.1";
        db_user = "root";
        db_passwd = "123456";
        db_name = "Online_Res_DB";
        port = 3306;
    }
    mysql_client(string host, string user, string passwd, string dbname, int p = 3306)
        : db_host(host),
          db_user(user),
          db_passwd(passwd),
          db_name(dbname),
          port(p)
    {
    }

    ~mysql_client()
    {
        mysql_close(&mysql_con); // 关闭连接
    }
    bool mysql_con_ser();
    bool mysql_Register(const string &username, const string &passwd, const string &realname, const string &id_card, const string &usertel, GENDER gen);
    bool mysql_Login(const string &usertel, const string &passwd, string &username);
    bool mysql_User_Show_Ticket(Json::Value &resval);
    bool mysql_User_Book_Ticket(int ticket_id, string usertel, int num);
    bool mysql_User_Show_My_Ticlet(Json::Value &resval,string usertel);
    bool mysql_User_Cancel_Ticket(int ticket_id, string usertel, int num);

private:
    MYSQL mysql_con;  // 初始数据库对象
    string db_host;   // 数据库地址
    string db_user;   // 数据库用户名
    string db_passwd; // 数据库密码
    string db_name;   // 数据库名
    int port;

    bool mysql_user_begin();
    bool mysql_user_commit();
    bool mysql_user_rollback();
};
