#pragma once

#include <jsoncpp/json/json.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <iomanip>

#include "../common/OpType.h"

using namespace std;

const int OFFSET = 2;

class socket_client
{
public:
    socket_client()
        : sockfd(-1)
        , port(6789)
        , dl_flg(false)
        , user_op(0)
        , runing(true)
        , gen(GENDER::ERR)
    {
        ips = "127.0.0.1";
    }

    socket_client(string ips, short port)
        : sockfd(-1)
        , ips(ips)
        , port(port)
        , dl_flg(false)
        , user_op(0)
        , runing(true)
        , gen(GENDER::ERR)
    {
    }

    ~socket_client()
    {
        close(sockfd);
    }

    bool connect_server();
    void Run();

    void User_Register();
    void User_Login();
    void User_Show_Ticket();
    void User_Book_Ticket();
    void User_Show_My_Ticlet();
    void User_Cancel_Ticket();

    bool send_json_msg(int sockfd, const Json::Value& val);

private:
    void print_info();

private:
    string ips;
    short port;
    int sockfd;
    bool dl_flg;
    int user_op;
    bool runing;

    string username;
    string usertel;
    string passwd;
    string realname;
    GENDER gen;
    string id_card;

    Json::Value m_val;
};
