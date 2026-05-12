#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include<jsoncpp/json/json.h>
#include "../common/OpType.h"
#include <iomanip>   // setw / left

using namespace std;
const int OFFSET=2;

class socket_client
{
public:
    socket_client() // 无参构造函数：如果你创建对象时不传参数，就用这套默认值
    {
        sockfd = -1;
        ips = "127.0.0.1";
        port = 6789;
        dl_flg = false;
        user_op = 0;
        runing=true;
    }

    socket_client(string ips, short port) // 有参构造函数
    {
        sockfd = -1;
        this->ips = ips;
        this->port = port;
        dl_flg = false;
        user_op = 0;
        runing=true;
    }

    void print_info();

    bool connect_server();

    void Run();

    void User_Register();//注册
    void User_Login();   //登录
    void User_Show_Ticket();//查看预约信息
    void User_Book_Ticket();//预定操作
    void User_Show_My_Ticlet();//查看我的预约信息
    void User_Cancel_Ticket();//取消预约

    bool send_json_msg(int sockfd, const Json::Value &val);//封装Length-Value

    ~socket_client()
    {
        close(sockfd);
    }

private:
    string ips;
    short port;
    int sockfd;

    bool dl_flg;

    /*登录，注册的报文变量*/
    string username; // 用户名
    string usertel;  // 电话
    string passwd;   //密码
    string realname; //真实姓名
    GENDER gen;      //性别：0未知 1男 2女
    string id_card;  //身份证号

    int user_op; // 记录用户的选择
    bool runing;

    Json::Value m_val;
};
